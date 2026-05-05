#ifndef KIKI_INTERFACE_BUTTONCLICKEVENT
#define KIKI_INTERFACE_BUTTONCLICKEVENT

#include <entt/entt.hpp>

namespace Kiki {
    enum class ClickType {
        BUTTON_DOWN,
        BUTTON_RELEASE
    };

    struct ButtonClickEvent {
        entt::entity button;
        ClickType clickType;
    };
}

#endif