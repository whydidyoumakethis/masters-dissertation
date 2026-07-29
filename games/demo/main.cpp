#include <kiki.h>

#include "DemoCameraPathSystem.hpp"

int main(int argc, char** argv) {
    Kiki::Engine engine;
    engine.Init();

    Kiki::SceneManager::get().loadScene(Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza_diss.glb"));

    engine.RegisterSystem<DemoCameraPathSystem>();
    engine.Run();
}
