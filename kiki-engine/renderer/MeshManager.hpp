#ifndef KIKI_RENDERER_MESHMANAGER
#define KIKI_RENDERER_MESHMANAGER

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

    struct Mesh {

    };

    class MeshManager {
        public:
        static MeshManager& get();

        int createMesh(std::vector<Vertex> vertices, std::vector<uint32_t> indices);
        Mesh getMesh(int id);

        private:
        MeshManager() = default;
        ~MeshManager() = default;
        MeshManager(const MeshManager&) = delete;
        MeshManager& operator=(const MeshManager&) = delete;

        std::vector<Mesh> meshes;
    };
}

#endif