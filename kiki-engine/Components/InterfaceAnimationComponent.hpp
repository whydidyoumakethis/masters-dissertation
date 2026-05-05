#ifndef KIKI_COMPONENTS_INTERFACEANIMATIONCOMPONENT
#define KIKI_COMPONENTS_INTERFACEANIMATIONCOMPONENT

#include "InterfaceComponent.hpp"
#include "BackgroundComponent.hpp"
#include "TextComponent.hpp"
#include "InterfaceTextureComponent.hpp"

#include "ECS/World.h"

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

    glm::vec4 targetTextureColour;
    
    bool loop = false;
    bool reverse = false;
    Kiki::InterfaceInterpolationType interpolation = Kiki::InterfaceInterpolationType::LINEAR;
    float time;
    float delay;

    // Not to be set manually
    bool init = false;
    bool reversing = false;
    float elapsed = 0.0f;
    float delayElapsed = 0.0f;

    ScaleVec2D positionDiff;
    ScaleVec2D sizeDiff;
    float rotationDiff = 0.0f;

    glm::vec3 backgroundColourDiff;
    float backgroundTransparencyDiff;

    glm::vec3 textColourDiff;
    float textTransparencyDiff;
    float textSizeDiff;

    glm::vec4 textureColourDiff;

    void copy(entt::entity e) {
        World& world = World::Get();
        auto& registry = world.Registry();

        if (registry.all_of<InterfaceComponent>(e)) {
            auto& interfaceComponent = registry.get<InterfaceComponent>(e);

            targetPosition = interfaceComponent.position;
            targetSize = interfaceComponent.size;
            targetRotation = interfaceComponent.rotation;
        }

        if (registry.all_of<BackgroundComponent>(e)) {
            auto& backgroundComponent = registry.get<BackgroundComponent>(e);

            targetBackgroundColour = backgroundComponent.colour;
            targetBackgroundTransparency = backgroundComponent.transparency;
        }

        if (registry.all_of<TextComponent>(e)) {
            auto& textComponent = registry.get<TextComponent>(e);

            targetTextColour = textComponent.colour;
            targetTextTransparency = textComponent.transparency;
            targetTextSize = textComponent.size;
        }

        if (registry.all_of<InterfaceTextureComponent>(e)) {
            auto& textureComponent = registry.get<InterfaceTextureComponent>(e);

            targetTextureColour = textureComponent.colour;
        }
    }
};

#endif