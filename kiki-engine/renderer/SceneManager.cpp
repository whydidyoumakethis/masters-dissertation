#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <volk.h>
#include <glm/gtx/matrix_decompose.hpp> 

#include "physics/PhysicsComponents.hpp" 
#include "physics/PhysicsUtils.hpp"   
#include "physics/PhysicsSystem.hpp" 

#include "Animation/AnimationLoader.h"
#include "Animation/AnimationComponent.h"

#include <assimp/Importer.hpp> 

#include <spdlog/spdlog.h>

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

     int SceneManager::createMaterial(Mtexture& texture) {
         materials.emplace_back(RenderManager::get().allocateMaterial(texture));

         return materials.size() - 1;
     }

    Material const& SceneManager::getMaterial(int id) {
        return materials[id];
    }

    int SceneManager::createMesh(
        std::vector<glm::vec3> const& positions,
        std::vector<std::uint32_t> const& indices,
        std::vector<glm::vec3> const& normals,
        std::vector<glm::vec2> const& texCoords,
        std::vector<glm::ivec4> const& boneIDs,  
        std::vector<glm::vec4> const& weights  
    )
    {
        std::vector<float> p, t, n;

        for (glm::vec3 pos : positions) {
            p.emplace_back(pos.x);
            p.emplace_back(pos.y);
            p.emplace_back(pos.z);
        }

        for (glm::vec2 texCoord : texCoords) {
            t.emplace_back(texCoord.x);
            t.emplace_back(texCoord.y);
        }

        for (glm::vec3 norm : normals) {
            n.emplace_back(norm.x);
            n.emplace_back(norm.y);
            n.emplace_back(norm.z);
        }

        // 处理骨骼数据 (平坦化处理)
        std::vector<int> bIDs;
        std::vector<float> bWeights;

        for (const auto& id : boneIDs) {
            bIDs.push_back(id.x); bIDs.push_back(id.y); bIDs.push_back(id.z); bIDs.push_back(id.w);
        }
        for (const auto& w : weights) {
            bWeights.push_back(w.x); bWeights.push_back(w.y); bWeights.push_back(w.z); bWeights.push_back(w.w);
        }

        spdlog::info("Allocating Mesh -> Pos: {}, Indices: {}, Normals: {}, UVs: {}, Bones: {}, Weights: {}",
            p.size(), indices.size(), n.size(), t.size(), bIDs.size(), bWeights.size());

        // 重点：RenderManager::allocateMesh 也需要相应修改签名
        meshes.emplace_back(RenderManager::get().allocateMesh(p, indices, n, t, bIDs, bWeights));

        return meshes.size() - 1;
    }

    Mesh& SceneManager::getMesh(int id) {
        return meshes[id];
    }

    void SceneManager::clearLevel() {
        vkDeviceWaitIdle(RenderManager::get().getDevice());

        auto& registry = World::Get().Registry();

        auto view = World::Get().Query<MeshComponent>();

        for (auto [e, meshComponent] : view.each()) {
            registry.destroy(e);
        }

        materials.clear();
        meshes.clear();
    }

    entt::entity SceneManager::loadModel(const std::string path, const std::string name, PhysicsType type) {
        auto model = World::Get().CreateEntity();
        auto& registry = World::Get().Registry();

        std::string fullPath = (std::filesystem::path(PROJECT_ASSETS_PATH) / path).string();

        // ==========================================
        // 1. [新增] 预检并加载动画数据
        // ==========================================
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(fullPath, ASSIMP_FLAGS);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            spdlog::error("Failed to load glTF file: {} with error: {}", fullPath, importer.GetErrorString());
            return model;
        }

        std::unique_ptr<Skeleton> skeleton = nullptr;
        std::unique_ptr<Animation> animation = nullptr;

        // 只有当场景包含动画时，才去解析骨骼和动画曲线
        if (scene->HasAnimations()) {
            skeleton = AnimationLoader::LoadSkeleton(scene);
            if (skeleton) {
                // 默认加载第一个动画 (index 0)
                animation = AnimationLoader::LoadAnimation(scene, *skeleton, 0);
            }
        }

        // ==========================================
        // 2. [修改] 加载 Mesh 和 Texture
        // ==========================================
        // 技巧：如果 skeleton 为空，我们传一个局部的 dummy Skeleton。
        // 因为你的 GltfLoaderAssimp 会在找不到 bone 时 continue，所以 dummy skeleton 会完美返回全 0 的 weights 和 IDs。
        Skeleton dummySkeleton;
        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(fullPath, 0, skeleton ? *skeleton : dummySkeleton);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(fullPath, 0);

        // ==========================================
        // 3. 基础组件赋值
        // ==========================================
        registry.emplace<TransformComponent>(model);
        registry.emplace<TagComponent>(model, entt::hashed_string(name.c_str()), name);

        // [修改] 调用更新后的 createMesh，传入 boneIDs 和 weights
        registry.emplace<MeshComponent>(model, createMesh(
            mesh.vertices,
            mesh.indices,
            mesh.normals,
            mesh.uvs,
            mesh.boneIDs, // 新增
            mesh.weights  // 新增
        ));

        // ==========================================
        // 4. [新增] 挂载动画组件
        // ==========================================
        if (skeleton && animation) {
            auto& animComp = registry.emplace<AnimationComponent>(model);
            animComp.skeleton = std::move(skeleton);
            animComp.currentAnimation = std::move(animation);

            // [修改] 强制更新一次第 0 帧，确保 finalMatrices 初始化完成
            animComp.animator.Update(0.0f, *animComp.skeleton, *animComp.currentAnimation);

            // [实现 TODO] 真正分配 GPU 资源 
            animComp.boneMatrixBuffer = RenderManager::get().allocateAnimationBuffer();
            animComp.descriptorSet = RenderManager::get().allocateAnimationDescriptorSet(animComp.boneMatrixBuffer);

            // [新增] 立刻把第一帧的骨骼数据推送到刚创建的 GPU Buffer 中
            animComp.UpdateGpuBuffer(RenderManager::get().allocator.allocator);

            spdlog::info("Animation loaded successfully for entity: {}", name);
        }

        // ==========================================
        // 5. 物理与材质 (保持你原有的逻辑不变)
        // ==========================================
        JPH::Ref<JPH::Shape> colliderShape;
        JPH::EMotionType joltMotionType;
        uint16_t joltLayer;

        switch (type) {
        case PhysicsType::Static:
            colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
            joltMotionType = JPH::EMotionType::Static;
            joltLayer = 0; // NON_MOVING
            break;

        case PhysicsType::Dynamic:
            colliderShape = CreateConvexHull(mesh.vertices);
            joltMotionType = JPH::EMotionType::Dynamic;
            joltLayer = 1; // MOVING
            break;

        case PhysicsType::Kinematic:
            colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
            joltMotionType = JPH::EMotionType::Kinematic;
            joltLayer = 0;
            break;
        }

        if (colliderShape) {
            registry.emplace<MeshColliderComponent>(model, colliderShape);
            registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer);
            registry.emplace<PhysicalAttributesComponent>(model);
            spdlog::info("Model {} loaded as {}", path,
                type == PhysicsType::Static ? "Static" : (type == PhysicsType::Dynamic ? "Dynamic" : "Kinematic"));
        }

        if (texture.hastexture) {
            registry.emplace<MaterialComponent>(model, createMaterial(texture));
        }

        registry.emplace<ColourComponent>(model, glm::vec3(1, 0, 0));

        // Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
        // Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

        return model;
    }
   // void SceneManager::loadModel(const std::string modelName, int index) {
   //     auto model = World::Get().CreateEntity();
   //     auto& registry = World::Get().Registry();

   //     Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName, index);
   //     Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName, mesh.matIndex);
   //     registry.emplace<TransformComponent>(model);
   //     registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs));

   //     auto staticShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
   //     if (staticShape) {
   //         registry.emplace<MeshColliderComponent>(model, staticShape);
			////now assume all models are static...
   //         registry.emplace<RigidBodyComponent>(
   //             model,
   //             JPH::EMotionType::Static,
   //             0, 
   //             0.0f, 
   //             0.5f 
   //         );
   //     }

   //     if (texture.hastexture) {
   //         materials.emplace_back(RenderManager::get().allocateMaterial(texture));
   //         int id = materials.size() - 1;

   //         registry.emplace<MaterialComponent>(model, id);
   //     }

   //     registry.emplace<ColourComponent>(model, glm::vec3(0.3f, 0.3f, 0.3f));

   //     Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
   //     Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
   // }


    void SceneManager::loadScene(const Mscene& scene) {
        // TEMP SOLUTION

        for (int i = 0; i < scene.instances.size(); i++) {
           auto model = World::Get().CreateEntity();
            auto& registry = World::Get().Registry();
			const auto& instance = scene.instances[i];
            const Mmesh& mesh = scene.meshes[instance.meshIndex];
            const Mtexture& texture = scene.textures[mesh.matIndex];
            auto& transform = registry.emplace<TransformComponent>(model);
            glm::vec3 skew;
            glm::vec4 perspective;
			glm::decompose(scene.instances[i].transform, transform.scale, transform.rotation, transform.position, skew, perspective);
            transform.rotation = glm::conjugate(transform.rotation);
            transform.scale = {1, 1, 1}; // TODO: this is a temp fix, will probably cause issues

            std::vector<glm::ivec4> safeBoneIDs = mesh.boneIDs;
            std::vector<glm::vec4> safeWeights = mesh.weights;

            if (safeBoneIDs.empty() || safeWeights.empty()) {
                safeBoneIDs.assign(mesh.vertices.size(), glm::ivec4(0));
                safeWeights.assign(mesh.vertices.size(), glm::vec4(0.0f));
            }

            registry.emplace<MeshComponent>(model, createMesh(
                mesh.vertices,
                mesh.indices,
                mesh.normals,
                mesh.uvs,
                safeBoneIDs,  // 新增
                safeWeights   // 新增
            ));

            if (texture.hastexture) {
                materials.emplace_back(RenderManager::get().allocateMaterial(texture));
				int id = materials.size() - 1;
                registry.emplace<MaterialComponent>(model, id);
				if (texture.mode == alphaMode::MASK) {
                    registry.emplace<TransparencyComponent>(model); // yeah idk what else to do other then just have this added
                }
            }
            registry.emplace<ColourComponent>(model, glm::vec3(0.3f, 0.3f, 0.3f));
            Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
			Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
        }
    }

    bool SceneManager::validMaterial(int id) {
        return id >= 0 && id < materials.size();
    }

    bool SceneManager::validMesh(int id) {
        return id >= 0 && id < meshes.size();
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}