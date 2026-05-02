#include <kiki.h>

#include "systems/UISystem.hpp"

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

	engine.RegisterSystem<UISystem>();

	engine.Run();
}