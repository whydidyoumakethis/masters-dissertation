#ifndef KIKI_DEBUGGING_DEBUGSYSTEM
#define KIKI_DEBUGGING_DEBUGSYSTEM

#include "ECS/System.h"
#include "DebugInterface.hpp"
#include "DebugCamera.hpp"

namespace Kiki {
	class DebugSystem : public System {
        private:
        DebugInterface& interface = DebugInterface::get();
        DebugCamera& cam = DebugCamera::get();

        public:
        Phase GetPhase() const override { return Phase::PostUpdate; }

        void OnStart() override;

        void OnUpdate(float dt) override;

        void OnStop() override;
	};
}

#endif