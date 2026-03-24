#include <kiki.h>

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

    // Kiki::SceneManager::get().loadModel("donut_cube.glb");
    // Kiki::SceneManager::get().loadModel("road.glb");

    Kiki::SceneManager::get().loadScene("sponza.glb");

    engine.Run();
}
