#ifndef KIKI_COMPONENTS_INTERFACECOMPONENT
#define KIKI_COMPONENTS_INTERFACECOMPONENT

#include "interface/InterfaceSystem.hpp"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>

struct InterfaceComponent {
    Kiki::ScaleVec2D position;
    Kiki::ScaleVec2D size;
    entt::entity parent = entt::null;
    unsigned int zindex = 1;
    
    bool dirty = true;
};

#endif