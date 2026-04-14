#include "DebugSystem.hpp"

namespace Kiki {
    void DebugSystem::OnStart() {
#	    ifndef NDEBUG
        World::Get().GetComponent<TagComponent>(cam.camera)->tag = entt::hashed_string("DebugCamera");
        World::Get().GetComponent<TagComponent>(cam.camera)->name = "DebugCamera";
        interface.initialise();
#       endif
    }

    void DebugSystem::OnUpdate(float dt) {
#	    ifndef NDEBUG
        if (cam.enabled) 
            cam.update(dt);
            
        interface.update(dt);
#       endif
    }

    void DebugSystem::OnStop() {
#	    ifndef NDEBUG
        interface.shutdown();
#       endif
    }
}