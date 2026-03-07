#include "MaterialManager.hpp"

namespace Kiki {
    MaterialManager& MaterialManager::get() {
        static MaterialManager instance;
        return instance;
    }
}