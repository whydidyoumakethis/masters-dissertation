#ifndef KIKI_COMPONENTS_BUTTONCOMPONENT
#define KIKI_COMPONENTS_BUTTONCOMPONENT

#include "interface/InterfaceSystem.hpp"

#include <glm/vec4.hpp>
enum ButtonState {
    NONE,
    HOVER,
    BUTTON_DOWN
};
struct ButtonComponent {
    glm::vec4 colour;
    glm::vec4 hoverColour;
    glm::vec4 clickColour;
    ButtonState buttonState = ButtonState::NONE;
};

#endif