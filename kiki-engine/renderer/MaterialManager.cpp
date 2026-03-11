#include "MaterialManager.hpp"

namespace Kiki {
    MaterialManager& MaterialManager::get() {
        static MaterialManager instance;
        return instance;
    }

    int MaterialManager::createMaterial(std::filesystem::path texture, BlendMode blendMode) {
        materials.emplace_back(RenderManager::get().allocateMaterial(texture, blendMode));

        return materials.size() - 1;
    }

    Material const& MaterialManager::getMaterial(int id) {
        return materials[id];
    }
}