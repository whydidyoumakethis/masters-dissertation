#include <kiki.h>

#include "component/CharacterComponent.h"
#include "system/CharacterSystem.h"
#include "system/ThirdPersonCameraSystem.h"
#include "system/GoalTriggerSystem.h"
#include "system/TimeLimitSystem.h"
#include "Components/BackgroundComponent.hpp"
#include "Components/InterfaceComponent.hpp"
#include "Components/TextComponent.hpp"
#include "Components/ButtonComponent.hpp"
#include "interface/FontManager.hpp"

#include "GltfLoader/GltfLoaderAssimp.h"

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();
  
  	auto& world = World::Get();
	auto& sceneManager = Kiki::SceneManager::get();
    //auto cube = sceneManager.loadModel("CesiumMan.glb", "Cube", PhysicsType::Dynamic);
    //auto objects = world.Query<TransformComponent,TagComponent>();

	Mscene scene = Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza.glb");
	Kiki::SceneManager::get().loadScene(std::move(scene));

	Mscene player = Kiki::GltfLoaderAssimp::loadScene(std::filesystem::path(PROJECT_ASSETS_PATH) / "demo_level2.glb");
	Kiki::SceneManager::get().loadScene(std::move(player));

	// example of setting a custom skybox
	Kiki::RenderManager::get().setCustomSkybox(
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_right.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_left.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_up.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_down.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_front.png",
		std::filesystem::path(PROJECT_ROOT_PATH) / "games/demo/assets/custom_skybox_back.png"
	);

	// auto& registry = world.Registry();
	// auto ui = world.CreateEntity();
	// registry.emplace<InterfaceComponent>(ui, Kiki::ScaleVec2D(0.2, 0, 0.4, 0), Kiki::ScaleVec2D(0.6, 0, 0.2, 0));
	// registry.emplace<BackgroundComponent>(ui, glm::vec3(1.0f, 0.0f, 0.0f), 0.5f);
	// registry.emplace<ButtonComponent>(ui, glm::vec4(1.0f, 0.0f, 0.0f, 0.5f), glm::vec4(0.0f, 1.0f, 0.0f, 0.5f), glm::vec4(0.0f, 0.0f, 1.0f, 0.5f));

	 Kiki::FontManager::get().loadFont(std::filesystem::path(PROJECT_ASSETS_PATH) / "fonts/NotoSans-Regular.ttf", "font");// , U"お茶ください");
	// registry.emplace<TextComponent>(ui, "font", U"お茶ください", 48.0f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f);

	// auto ui2 = world.CreateEntity();
	// registry.emplace<InterfaceComponent>(ui2, Kiki::ScaleVec2D(0.4, 0, 0.2, 0), Kiki::ScaleVec2D(0.2, 0, 0.6, 0), ui, (unsigned int) 1);
	// registry.emplace<BackgroundComponent>(ui2, glm::vec3(0.0f, 1.0f, 0.0f), 0.0f);

	// resigster after loading the character component to avoid potential issues with systems trying to access the character component before it's added to the entity
	
	engine.RegisterSystem<TimeLimitSystem>();
	// use wasd to move the character, shift to speed up, space to jump.
	engine.RegisterSystem<CharacterSystem>();
	engine.RegisterSystem<ThirdPersonCameraSystem>();
	engine.RegisterSystem<GoalTriggerSystem>();
    engine.Run();
}
