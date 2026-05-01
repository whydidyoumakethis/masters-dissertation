#ifndef KIKI_COMPONENTS_INTERFACEANIMATIONCOMPONENT
#define KIKI_COMPONENTS_INTERFACEANIMATIONCOMPONENT

#include "InterfaceComponent.hpp"

namespace Kiki {
    enum InterfaceInterpolationType {
        LINEAR,
        EASE_IN,
        EASE_OUT,
        EASE_IN_OUT
    };
}

struct InterfaceAnimationComponent {
    ScaleVec2D targetPosition;
    ScaleVec2D targetSize;
    float targetRotation = 0.0f;

    glm::vec3 targetBackgroundColour;
    float targetBackgroundTransparency;

    glm::vec3 targetTextColour;
    float targetTextTransparency;
    float targetTextSize;
    
    bool loop = false;
    bool reverse = false;
    Kiki::InterfaceInterpolationType interpolation = Kiki::InterfaceInterpolationType::LINEAR;
    float time;

    // Not to be set manually
    bool init = false;
    bool reversing = false;
    float elapsed = 0.0f;

    ScaleVec2D positionDiff;
    ScaleVec2D sizeDiff;
    float rotationDiff = 0.0f;

    glm::vec3 backgroundColourDiff;
    float backgroundTransparencyDiff;

    glm::vec3 textColourDiff;
    float textTransparencyDiff;
    float textSizeDiff;
};

#endif