#include <kiki.h>

#include "DemoCameraPathSystem.hpp"

int main(int argc, char** argv) {
    Kiki::Engine engine;
    engine.Init();

    engine.RegisterSystem<DemoCameraPathSystem>();

    std::thread([]() {
        Kiki::SceneManager::get().loadScene(Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza.glb"));
    }).detach();

    engine.Run();
}
