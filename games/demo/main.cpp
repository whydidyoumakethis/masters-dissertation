#include <kiki.h>

int main(int argc, char** argv) {
   

	Engine engine;
	engine.Init();
	auto& world = World::Get();
	auto& sceneManager = Kiki::SceneManager::get();
    auto cube = sceneManager.loadModel("test_cube_tex.glb", "Cube", PhysicsType::Dynamic);
    auto objects = world.Query<TransformComponent,TagComponent>();
    for (auto [e, transform, tag] : objects.each()) {
		if (tag.tag == "Cube"_hs) {
            transform.position.y = 20;
			transform.dirty = true;
        }
    }
    sceneManager.loadModel("road.glb");
    engine.Run();

}
