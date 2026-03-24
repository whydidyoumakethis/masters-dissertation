#include <kiki.h>

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

    Kiki::SceneManager::get().loadModel("test_cube_tex.glb");
    Kiki::SceneManager::get().loadModel("road.glb");

    engine.Run();
}
