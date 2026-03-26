#ifndef KIKI_RENDERER_SCENEMANAGER
#define KIKI_RENDERER_SCENEMANAGER

#include "RenderManager.hpp"

#include <vector>
#include <filesystem>

namespace Kiki {

    enum class PhysicsType {
        Static, 
        Dynamic, 
        Kinematic
    };

    class SceneManager {
        public:
        static SceneManager& get();

        int createMaterial(stbi_uc* imageData, int baseWidthi, int baseHeighti);
        Material const& getMaterial(int id);

        int createMesh(std::vector<glm::vec3> const& positions, std::vector<std::uint32_t> const& indices, std::vector<glm::vec2> const& texCoords);
        Mesh& getMesh(int id);

        void clearLevel();
        void shutdown();

        void loadModel(const std::string path, const std::string name = "Object", PhysicsType type = PhysicsType::Static);

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