#include "MeshManager.hpp"

namespace Kiki {
    MeshManager& MeshManager::get() {
        static MeshManager instance;
        return instance;
    }
}