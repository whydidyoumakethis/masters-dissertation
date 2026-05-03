#include <kiki.h>

#include "systems/UISystem.hpp"
#include "systems/LevelSystem.hpp"
#include "systems/CharacterSystem.h"
#include "systems/ThirdPersonCameraSystem.h"
#include "systems/GoalTriggerSystem.h"
#include "systems/TimeLimitSystem.h"

int main(int argc, char** argv) {
	Kiki::Engine engine;
	engine.Init();

	engine.RegisterSystem<LevelSystem>();
	engine.RegisterSystem<UISystem>();
	engine.RegisterSystem<CharacterSystem>();
	engine.RegisterSystem<ThirdPersonCameraSystem>();
	engine.RegisterSystem<GoalTriggerSystem>();
	engine.RegisterSystem<TimeLimitSystem>();

	engine.Run();
}