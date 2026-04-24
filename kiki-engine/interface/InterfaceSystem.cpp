#include "InterfaceSystem.hpp"

#include "Components/InterfaceComponent.hpp"

namespace Kiki {
    void InterfaceSystem::OnStart() {
        fontManager.initialise();
    }

    // Update position of InterfaceComponents and fire any events
    void InterfaceSystem::OnUpdate(float dt) {

    }

    void InterfaceSystem::OnStop() {
        fontManager.shutdown();
    }
} 
