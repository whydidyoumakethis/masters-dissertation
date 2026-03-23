#include "SceneManager.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
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

    int SceneManager::createMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> texCoords) {
        meshes.emplace_back(RenderManager::get().allocateMesh(positions, indices, texCoords));

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
    void SceneManager::loadModel(const std::string modelName) {
        auto model = World::Get().CreateEntity();
        auto& registry = World::Get().Registry();

        Mmesh mesh = Kiki::GltfLoaderAssimp::loadMesh(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName);
        Mtexture texture = Kiki::GltfLoaderAssimp::loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / modelName);
        registry.emplace<TransformComponent>(model);
        registry.emplace<MeshComponent>(model, createMesh(mesh.vertices, mesh.indices, mesh.uvs));
        const bool isCompressed = !texture.rawData.empty();
        unsigned char* texPtr = isCompressed
            	? (unsigned char*)texture.rawData.data()
            	: (unsigned char*)texture.data.data();
        int texSize = isCompressed
            	? static_cast<int>(texture.rawData.size())
            	: static_cast<int>(texture.data.size() * sizeof(RGBA));

        registry.emplace<MaterialComponent>(model,
            	createMaterial(texture.rawDataPtr, texture.width, texture.height, BlendMode::OPAQUE));

        Kiki::GltfLoaderAssimp::debugPrintMesh(mesh);
        Kiki::GltfLoaderAssimp::debugPrintTexture(texture);
    }
    void SceneManager::shutdown() {
        materials.clear();
        meshes.clear();
    }
}