#ifndef KIKI_RENDERER_MATERIALMANAGER
#define KIKI_RENDERER_MATERIALMANAGER

#include "RenderManager.hpp"

#include <vector>
#include <filesystem>

namespace Kiki {

    class MaterialManager {
        public:
        static MaterialManager& get();

        int createMaterial(std::filesystem::path texture, BlendMode blendMode = BlendMode::OPAQUE);
        Material const& getMaterial(int id);
        void shutdown();

        private:
        MaterialManager() = default;
        ~MaterialManager() = default;
        MaterialManager(const MaterialManager&) = delete;
        MaterialManager& operator=(const MaterialManager&) = delete;

        std::vector<Material> materials;
    };
}

#endif