#include <kiki.h>

#include "component/CharacterComponent.h"
#include "system/CharacterSystem.h"
#include "system/ThirdPersonCameraSystem.h"
#include "system/GoalTriggerSystem.h"

#include "GltfLoader/GltfLoaderAssimp.h"

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();
  
  	auto& world = World::Get();
	auto& sceneManager = Kiki::SceneManager::get();

	Mscene scene = Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "demo_level.glb");
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


	// resigster after loading the character component to avoid potential issues with systems trying to access the character component before it's added to the entity
	// use wasd to move the character, shift to speed up, space to jump.
	engine.RegisterSystem<CharacterSystem>();
	// use left click to disable cursor and control camera, esc to quit.
	engine.RegisterSystem<ThirdPersonCameraSystem>();
	engine.RegisterSystem<GoalTriggerSystem>();
    engine.Run();
}
