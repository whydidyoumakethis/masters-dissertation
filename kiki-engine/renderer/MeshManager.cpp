#include "MeshManager.hpp"

#include "utils/Buffer.hpp"
#include "../logging/FatalError.hpp"


namespace Kiki {
    MeshManager& MeshManager::get() {
        static MeshManager instance;
        return instance;
    }

    int MeshManager::createMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices) {
        
    }

    Mesh* MeshManager::getMesh(int id) {
        return &meshes[id];
    }
}