#include "SceneManager.hpp"

namespace Kiki {
    SceneManager& SceneManager::get() {
        static SceneManager instance;
        return instance;
    }

    int SceneManager::createMaterial(std::filesystem::path texture, BlendMode blendMode) {
        materials.emplace_back(RenderManager::get().allocateMaterial(texture, blendMode));

        return materials.size() - 1;
    }

    Material const& SceneManager::getMaterial(int id) {
        return materials[id];
    }

    int SceneManager::createMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> texCoords) {
        meshes.emplace_back(RenderManager::get().allocateMesh(positions, indices, texCoords));

        return meshes.size() - 1;
    }

    Mesh& SceneManager::getMesh(int id) {
        return meshes[id];
    }

    void SceneManager::clearLevel() {
        materials.clear();
        meshes.clear();
    }

    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}