#include "SceneManager.hpp"

#include <volk.h>

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

    int SceneManager::createMaterial(stbi_uc* imageData, int baseWidthi, int baseHeighti, BlendMode blendMode) {
        materials.emplace_back(RenderManager::get().allocateMaterial(imageData, baseWidthi, baseHeighti, blendMode));

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

        auto view = World::Get().Query<MeshComponent, MaterialComponent>();

        for (auto [e, meshComponent, materialComponent] : view.each()) {
            registry.destroy(e);
        }

        materials.clear();
        meshes.clear();
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}