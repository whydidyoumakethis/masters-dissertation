#include <kiki.h>

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

    Kiki::SceneManager::get().loadModel("donut_cube.glb");

    engine.Run();
}
