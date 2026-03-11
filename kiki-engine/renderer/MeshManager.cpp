#include "MeshManager.hpp"

#include "utils/Buffer.hpp"
#include "../logging/FatalError.hpp"


namespace Kiki {
    MeshManager& MeshManager::get() {
        static MeshManager instance;
        return instance;
    }

    int MeshManager::createMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> texCoords) {
        meshes.emplace_back(RenderManager::get().allocateMesh(positions, indices, texCoords));

        return meshes.size() - 1;
    }

    Mesh& MeshManager::getMesh(int id) {
        return meshes[id];
    }
    
    void MeshManager::shutdown() {
        meshes.clear();
    }
}