#include <kiki.h>

int main(int argc, char** argv) {
   

	Kiki::Engine engine;
	engine.Init();
    Kiki::SceneManager::get().loadModel("test_cube_tex.glb", "Cube", Kiki::PhysicsType::Dynamic);
    auto objects = World::Get().Query<TransformComponent,TagComponent>();
    for (auto [e, transform, tag] : objects.each()) {
		if (tag.tag == "Cube"_hs) {
            transform.position.y = 20;
			transform.dirty = true;
        }
    }
    Kiki::SceneManager::get().loadModel("road.glb");
    engine.Run();

}
