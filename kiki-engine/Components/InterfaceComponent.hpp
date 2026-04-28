#ifndef KIKI_COMPONENTS_INTERFACECOMPONENT
#define KIKI_COMPONENTS_INTERFACECOMPONENT

#include "interface/InterfaceSystem.hpp"

#include <entt/entt.hpp>
#include <glm/vec3.hpp>
struct ScaleVec2D {
    float scaleX;
    float x;
    float scaleY;
    float y;

    // Absolute values should only be changed by the InterfaceSystem
    float absoluteX;
    float absoluteY;
};
struct InterfaceComponent {
    ScaleVec2D position;
    ScaleVec2D size;
    entt::entity parent = entt::null;
    unsigned int zindex = 1;
    
    bool dirty = true;
};

#endif