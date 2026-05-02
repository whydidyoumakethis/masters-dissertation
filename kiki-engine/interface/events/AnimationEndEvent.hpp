#ifndef KIKI_INTERFACE_ANIMATIONENDEVENT
#define KIKI_INTERFACE_ANIMATIONENDEVENT

#include <entt/entt.hpp>

namespace Kiki {
    struct AnimationEndEvent {
        entt::entity entity;
    };
}

#endif