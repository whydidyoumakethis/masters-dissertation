#ifndef KIKI_INPUT_INPUTSYSTEM
#define KIKI_INPUT_INPUTSYSTEM

#include "ECS/System.h"

namespace Kiki {
	class InputSystem : public System {
	private:
		Kiki::InputManager& inputManager = Kiki::InputManager::get();

	public:
		Phase GetPhase() const override { return Phase::Input; }

		void OnStart() override;

		void OnUpdate(float dt) override;
	};
}

#endif
