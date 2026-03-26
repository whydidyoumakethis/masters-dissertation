#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <volk.h>
#include <glm/gtx/matrix_decompose.hpp> 

// 在 SceneManager.cpp 顶部添加
#include "physics/PhysicsComponents.hpp" // 包含 RigidBodyComponent
#include "physics/PhysicsUtils.hpp"      // 包含我们刚写的 CreateTriangleMesh
#include "physics/PhysicsSystem.hpp"     // 如果你需要直接调用系统初始化

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

    // int SceneManager::createMaterial(stbi_uc* imageData, int baseWidthi, int baseHeighti) {
    //     materials.emplace_back(RenderManager::get().allocateMaterial(imageData, baseWidthi, baseHeighti));

    //     return materials.size() - 1;
    // }

    Material const& SceneManager::getMaterial(int id) {
        return materials[id];
    }

    int SceneManager::createMesh(std::vector<glm::vec3> const& positions, std::vector<std::uint32_t> const& indices, std::vector<glm::vec3> const& normals, std::vector<glm::vec2> const& texCoords) {
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

        meshes.emplace_back(RenderManager::get().allocateMesh(p, indices, n, t));

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

    void SceneManager::loadModel(const std::string modelName, int index) {
        auto model = World::Get().CreateEntity();
        auto& registry = World::Get().Registry();

        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName, index);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName, mesh.matIndex);
        registry.emplace<TransformComponent>(model);
        registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs));

        auto staticShape = CreateTriangleMesh(mesh.vertices, mesh.indices);
        if (staticShape) {
            registry.emplace<MeshColliderComponent>(model, staticShape);
			//now assume all models are static...
            registry.emplace<RigidBodyComponent>(
                model,
                JPH::EMotionType::Static,
                0, 
                0.0f, 
                0.5f 
            );
        }

        if (texture.hastexture) {
            materials.emplace_back(RenderManager::get().allocateMaterial(texture));
            int id = materials.size() - 1;

            registry.emplace<MaterialComponent>(model, id);
        }

        registry.emplace<ColourComponent>(model, glm::vec3(0.3f, 0.3f, 0.3f));

        Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
        Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
    }


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

            registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.normals, mesh.uvs));
            if (texture.hastexture) {
                materials.emplace_back(RenderManager::get().allocateMaterial(texture));
				int id = materials.size() - 1;
                registry.emplace<MaterialComponent>(model, id);
            }
            registry.emplace<ColourComponent>(model, glm::vec3(0.3f, 0.3f, 0.3f));
            Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
			Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
        }
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}