#ifndef KIKI_COMPONENTS_BUTTONCOMPONENT
#define KIKI_COMPONENTS_BUTTONCOMPONENT

#include "interface/InterfaceSystem.hpp"

#include <glm/vec4.hpp>

struct ButtonComponent {
    glm::vec4 colour;
    glm::vec4 hoverColour;
    glm::vec4 clickColour;
    Kiki::ButtonState buttonState = Kiki::ButtonState::NONE;
};

#endif