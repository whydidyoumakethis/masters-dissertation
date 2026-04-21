#ifndef KIKI_COMPONENTS_INTERFACECOMPONENT
#define KIKI_COMPONENTS_INTERFACECOMPONENT

#include "interface/InterfaceSystem.hpp"

#include <entt/entt.hpp>

struct InterfaceComponent {
    Kiki::ScaleVec2D position;
    Kiki::ScaleVec2D size;
    entt::entity parent = entt::null;
};

#endif