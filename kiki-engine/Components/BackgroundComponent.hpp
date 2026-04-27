#ifndef KIKI_COMPONENTS_BACKGROUNDCOMPONENT
#define KIKI_COMPONENTS_BACKGROUNDCOMPONENT

#include "renderer/utils/Buffer.hpp"

#include <glm/vec3.hpp>

struct BackgroundComponent {
    glm::vec3 colour;
    float transparency;

    rutils::Buffer vertices;
};

#endif