#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include <volk.h>

// 在 SceneManager.cpp 顶部添加
#include "physics/PhysicsComponents.hpp" // 包含 RigidBodyComponent
#include "physics/PhysicsUtils.hpp"      // 包含我们刚写的 CreateTriangleMesh
#include "physics/PhysicsSystem.hpp"     // 如果你需要直接调用系统初始化

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

    int SceneManager::createMaterial(stbi_uc* imageData, int baseWidthi, int baseHeighti) {
        materials.emplace_back(RenderManager::get().allocateMaterial(imageData, baseWidthi, baseHeighti));

        return materials.size() - 1;
    }

    Material const& SceneManager::getMaterial(int id) {
        return materials[id];
    }

    int SceneManager::createMesh(std::vector<glm::vec3> const& positions, std::vector<std::uint32_t> const& indices, std::vector<glm::vec2> const& texCoords) {
        std::vector<float> p, t;

        for (glm::vec3 pos : positions) {
            p.emplace_back(pos.x);
            p.emplace_back(pos.y);
            p.emplace_back(pos.z);
        }

        for (glm::vec2 texCoord : texCoords) {
            t.emplace_back(texCoord.x);
            t.emplace_back(texCoord.y);
        }

        meshes.emplace_back(RenderManager::get().allocateMesh(p, indices, t));

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

    void SceneManager::loadModel(const std::string modelName) {
        auto model = World::Get().CreateEntity();
        auto& registry = World::Get().Registry();

        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName);
        registry.emplace<TransformComponent>(model);
        registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.uvs));

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
            registry.emplace<MaterialComponent>(model,
                    createMaterial(texture.rawDataPtr, texture.width, texture.height));
        }

        registry.emplace<ColourComponent>(model, glm::vec3(1, 0, 0));

        Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
        Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}