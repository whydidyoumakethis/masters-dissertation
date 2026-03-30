#include <kiki.h>
#include "component/CharacterComponent.h"
#include "system/CharacterSystem.h"
int main(int argc, char** argv) {
   

	Engine engine;
	engine.Init();
	auto& world = World::Get();
	auto& sceneManager = Kiki::SceneManager::get();
    auto cube = sceneManager.loadModel("test_cube_tex.glb", "Cube", PhysicsType::Dynamic);
    auto objects = world.Query<TransformComponent,TagComponent>();

	// when we have loadCharacter implemented, we can replace this with loading the character model and adding the CharacterComponent in the loadCharacter function
    for (auto [e, transform, tag] : objects.each()) {
		if (tag.tag == "Cube"_hs) {
            transform.position.y = 20;
			transform.position.z = -10;
			transform.dirty = true;
			world.Registry().emplace<CharacterComponent>(e);
        }
		if (tag.tag == "camera"_hs) {
			auto& cam = world.Registry().emplace<ThirdPersonCameraComponent>(e);
			cam.followTarget = cube;
		}
    }
	sceneManager.loadModel("road.glb");

	// resigster after loading the character component to avoid potential issues with systems trying to access the character component before it's added to the entity
	engine.RegisterSystem<CharacterSystem>();
    engine.Run();

}
