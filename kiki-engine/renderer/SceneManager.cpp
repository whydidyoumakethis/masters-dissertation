#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <volk.h>
#include <glm/gtx/matrix_decompose.hpp> 

#include "physics/PhysicsComponents.hpp" 
#include "physics/PhysicsUtils.hpp"   
#include "physics/PhysicsSystem.hpp" 
#include "Components/MiscComponent.hpp"
#include "Components/RoughnessMetallicFactorComponent.hpp"

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
        std::vector<glm::vec4> const& tangents,
        std::vector<glm::ivec4> const& boneIDs,  
        std::vector<glm::vec4> const& weights  
    )
    {
        std::vector<float> p, t, n, tangents_dst;

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

        std::vector<int> bIDs;
        std::vector<float> bWeights;

        for (const auto& id : boneIDs) {
            bIDs.push_back(id.x); bIDs.push_back(id.y); bIDs.push_back(id.z); bIDs.push_back(id.w);
        }
        for (const auto& w : weights) {
            bWeights.push_back(w.x); bWeights.push_back(w.y); bWeights.push_back(w.z); bWeights.push_back(w.w);
        }

        //spdlog::info("Allocating Mesh -> Pos: {}, Indices: {}, Normals: {}, UVs: {}, Bones: {}, Weights: {}", p.size(), indices.size(), n.size(), t.size(), bIDs.size(), bWeights.size());

        for (glm::vec4 tan : tangents) {
            tangents_dst.emplace_back(tan.x);
            tangents_dst.emplace_back(tan.y);
            tangents_dst.emplace_back(tan.z);
            tangents_dst.emplace_back(tan.w);
        }

        meshes.emplace_back(RenderManager::get().allocateMesh(p, indices, n, t, tangents_dst, bIDs, bWeights));

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

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(fullPath, ASSIMP_FLAGS);

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
            spdlog::error("Failed to load glTF file: {} with error: {}", fullPath, importer.GetErrorString());
            return model;
        }

        std::unique_ptr<Skeleton> skeleton = nullptr;
        std::unique_ptr<Animation> animation = nullptr;

        if (scene->HasAnimations()) {
            skeleton = AnimationLoader::LoadSkeleton(scene);
            if (skeleton) {
                animation = AnimationLoader::LoadAnimation(scene, *skeleton, 0);
            }
        }

        Skeleton dummySkeleton;
        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(fullPath, 0, skeleton ? *skeleton : dummySkeleton);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(fullPath, 0);

        registry.emplace<TransformComponent>(model);
        registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs, mesh.tangents, mesh.boneIDs, mesh.weights));
        registry.emplace<TagComponent>(model, entt::hashed_string(name.c_str()), name);

        registry.emplace<MeshComponent>(model, createMesh(
            mesh.vertices,
            mesh.indices,
            mesh.normals,
            mesh.uvs,
            mesh.tangents,
            mesh.boneIDs, 
            mesh.weights 
        ));

        if (skeleton && scene->HasAnimations()) {
            auto& animComp = registry.emplace<AnimationComponent>(model);
            animComp.skeleton = std::move(skeleton);

			// read all animations and assign them to the state machine based on their names
            for (unsigned int i = 0; i < scene->mNumAnimations; i++) {
                std::string animName = scene->mAnimations[i]->mName.C_Str();
                std::transform(animName.begin(), animName.end(), animName.begin(), ::tolower);

                spdlog::info("Found animation in glTF: {}", animName);

                if (animName.find("idle") != std::string::npos) {
                    animComp.animations[CharacterState::Idle] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
                else if (animName.find("walking") != std::string::npos) {
                    animComp.animations[CharacterState::Walking] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
                else if (animName.find("running") != std::string::npos) {
                    animComp.animations[CharacterState::Running] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
                else if (animName.find("jumping") != std::string::npos) {
                    animComp.animations[CharacterState::Jumping] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, i);
                }
            }

            animComp.ChangeState(CharacterState::Idle);

            if (animComp.animations.find(CharacterState::Idle) == animComp.animations.end()) {
                spdlog::warn("No 'Idle' animation found! Using the first available animation as default.");
                animComp.animations[CharacterState::Idle] = AnimationLoader::LoadAnimation(scene, *animComp.skeleton, 0);
            }

            animComp.animator.Update(0.0f, *animComp.skeleton, *animComp.animations[CharacterState::Idle]);

            animComp.boneMatrixBuffer = RenderManager::get().allocateAnimationBuffer();
            animComp.descriptorSet = RenderManager::get().allocateAnimationDescriptorSet(animComp.boneMatrixBuffer);
            animComp.UpdateGpuBuffer(RenderManager::get().allocator.allocator);
        }

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


    void SceneManager::loadScene(Mscene scene) {
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
            //transform.scale = {0.3, 0.3, 0.3}; // TODO: this is a temp fix, will probably cause issues

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
                mesh.tangents,
                safeBoneIDs, 
                safeWeights  
            ));

            if (texture.hastexture) {
                materials.emplace_back(RenderManager::get().allocateMaterial(texture));
				int id = materials.size() - 1;
                registry.emplace<MaterialComponent>(model, id);
				if (texture.mode == alphaMode::MASK) {
                    registry.emplace<TransparencyComponent>(model); // yeah idk what else to do other then just have this added
                }
            }
            registry.emplace<ColourComponent>(model, glm::vec3(texture.baseColour));
            registry.emplace<RoughnessMetallicFactorComponent>(model, glm::vec2(texture.roughnessFactor, texture.metallicFactor));
            //Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
			//Kiki::GltfLoaderAssimp::debugPrintTexture(texture);

            if (scene.hasAnimations && (instance.miscTag == MmiscTags::PLAYER)) {

                spdlog::info("Instance {} is a PLAYER with animations, attaching AnimationComponent...", i);

                auto& animComp = registry.emplace<AnimationComponent>(model);

                if (scene.skeleton) {
                    animComp.skeleton = std::move(scene.skeleton);
                }

                for (auto& [animName, animPtr] : scene.animations) {
                    if (animName.find("idle") != std::string::npos) {
                        animComp.animations[CharacterState::Idle] = std::move(animPtr);
                    }
                    else if (animName.find("walking") != std::string::npos) {
                        animComp.animations[CharacterState::Walking] = std::move(animPtr);
                    }
                    else if (animName.find("running") != std::string::npos) {
                        animComp.animations[CharacterState::Running] = std::move(animPtr);
                    }
                    else if (animName.find("jumping") != std::string::npos) {
                        animComp.animations[CharacterState::Jumping] = std::move(animPtr);
                    }
                }

                // set default state
                animComp.ChangeState(CharacterState::Idle);

                animComp.boneMatrixBuffer = RenderManager::get().allocateAnimationBuffer();
                animComp.descriptorSet = RenderManager::get().allocateAnimationDescriptorSet(animComp.boneMatrixBuffer);
                animComp.UpdateGpuBuffer(RenderManager::get().allocator.allocator);

                spdlog::info("AnimationComponent initialized for player successfully.");
            }

			// Physics setup

            registry.emplace<TagComponent>(model, entt::hashed_string(mesh.name.c_str()), mesh.name);
            JPH::Ref<JPH::Shape> colliderShape;
            JPH::EMotionType joltMotionType;
            uint16_t joltLayer;
            //COPIED WHAT WAS DONE IN LOAD MODEL, PLEASE WORK ON THIS FUNCTION FROM NOW AS PREVIOUS FUNCTION HAS DEPRECATED FUNCTIONS

			switch (instance.bodyType) {
			case MbodyType::STATIC:
                //colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                joltMotionType = JPH::EMotionType::Static;
                joltLayer = 0; // NON_MOVING
                break;
            case MbodyType::DYNAMIC:
                //colliderShape = CreateConvexHull(mesh.vertices);
                joltMotionType = JPH::EMotionType::Dynamic;
                joltLayer = 1; // MOVING
				break;
            case MbodyType::KINEMATIC:
                //colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                joltMotionType = JPH::EMotionType::Kinematic;
				joltLayer = 0;
                break;
             }

            switch (instance.colliderType) {

            case McolliderType::BOX:
                colliderShape = CreateConvexHull(mesh.vertices);
                break;
            case McolliderType::CONVEX_HULL:
                colliderShape = CreateConvexHull(mesh.vertices);
                break;
            case McolliderType::MESH:
                colliderShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
                break;
            }

            if (colliderShape) {
                registry.emplace<MeshColliderComponent>(model, colliderShape);
				registry.emplace<RigidBodyComponent>(model, joltMotionType, joltLayer);
                registry.emplace<PhysicalAttributesComponent>(model);
            }

			// Misc tags
            if (instance.miscTag != MmiscTags::NONE) {
                registry.emplace<MiscComponent>(model, instance.miscTag);
			}
        }
        for (int i = 0; i < scene.emptyInstances.size(); i++) {
            auto model = World::Get().CreateEntity();
            auto& registry = World::Get().Registry();
            const auto& instance = scene.emptyInstances[i];
            auto& transform = registry.emplace<TransformComponent>(model);
            glm::vec3 skew;
            glm::vec4 perspective;
            glm::decompose(instance.transform, transform.scale, transform.rotation, transform.position, skew, perspective);
            transform.rotation = glm::conjugate(transform.rotation);
            // Misc tags
            if (instance.miscTag != MmiscTags::NONE) {
                registry.emplace<MiscComponent>(model, instance.miscTag);
            }
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