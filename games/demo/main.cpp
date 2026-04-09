#include <kiki.h>

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

	Mscene scene = Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza.glb");
	Kiki::SceneManager::get().loadScene(std::move(scene));

	// example of setting a custom skybox
	Kiki::RenderManager::get().setCustomSkybox(
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_right.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_left.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_up.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_down.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_front.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_back.png"
	);

    engine.Run();
}
