#ifndef KIKI_COMPONENTS_BACKGROUNDCOMPONENT
#define KIKI_COMPONENTS_BACKGROUNDCOMPONENT

#include <glm/vec3.hpp>

struct BackgroundComponent {
    glm::vec3 colour;
    float transparency;
    float cornerRadius;
};

#endif