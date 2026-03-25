#include <kiki.h>

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

	Mscene scene = Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza.glb");
	Kiki::SceneManager::get().loadScene(std::move(scene));

    engine.Run();
}
