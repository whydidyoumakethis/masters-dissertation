#ifndef KIKI_RENDERER_MESHMANAGER
#define KIKI_RENDERER_MESHMANAGER

#include "RenderManager.hpp"
#include "utils/Buffer.hpp"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <vector>
#include <cstdint>

namespace Kiki {
    struct Vertex {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    class MeshManager {
        public:
        static MeshManager& get();

        int createMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> texCoords);
        Mesh& getMesh(int id);
        void shutdown();

        private:
        MeshManager() = default;
        ~MeshManager() = default;
        MeshManager(const MeshManager&) = delete;
        MeshManager& operator=(const MeshManager&) = delete;

        std::vector<Mesh> meshes;
    };
}

#endif