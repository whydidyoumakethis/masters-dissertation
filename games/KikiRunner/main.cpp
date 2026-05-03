#include <kiki.h>

#include "systems/UISystem.hpp"
#include "systems/LevelSystem.hpp"

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

	engine.RegisterSystem<LevelSystem>();
	engine.RegisterSystem<UISystem>();

	engine.Run();
}