#ifndef KIKI_RENDERER_SCENEMANAGER
#define KIKI_RENDERER_SCENEMANAGER

#include "RenderManager.hpp"

#include <vector>
#include <filesystem>

namespace Kiki {
    class SceneManager {
        public:
        static SceneManager& get();

        int createMaterial(unsigned char* buffer, int bufferLength, BlendMode blendMode = BlendMode::OPAQUE);
        Material const& getMaterial(int id);

        int createMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> texCoords);
        Mesh& getMesh(int id);

        void clearLevel();
        void shutdown();

        private:
        SceneManager() = default;
        ~SceneManager() = default;
        SceneManager(const SceneManager&) = delete;
        SceneManager& operator=(const SceneManager&) = delete;

        std::vector<Material> materials;
        std::vector<Mesh> meshes;
    };
}

#endif