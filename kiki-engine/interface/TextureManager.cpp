#include "TextureManager.hpp"

namespace Kiki {
    TextureManager& TextureManager::get() {
        static TextureManager instance;
        return instance;
    }
}