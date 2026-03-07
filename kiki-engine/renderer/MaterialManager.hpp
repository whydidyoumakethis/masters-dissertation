#ifndef KIKI_RENDERER_MATERIALMANAGER
#define KIKI_RENDERER_MATERIALMANAGER

#include <vector>
#include <filesystem>

namespace Kiki {
    enum BlendMode {
        OPAQUE,
        TRANSPARENT
    };

    struct Material {
        BlendMode blendMode;

    };

    class MaterialManager {
        public:
        static MaterialManager& get();

        int createMaterial(std::filesystem::path texture, BlendMode blendMode = BlendMode::OPAQUE);
        Material getMaterial(int id);

        private:
        MaterialManager() = default;
        ~MaterialManager() = default;
        MaterialManager(const MaterialManager&) = delete;
        MaterialManager& operator=(const MaterialManager&) = delete;

        std::vector<Material> materials;
    };
}

#endif