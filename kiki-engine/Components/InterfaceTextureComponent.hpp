#ifndef KIKI_COMPONENTS_INTERFACETEXTURECOMPONENT
#define KIKI_COMPONENTS_INTERFACETEXTURECOMPONENT

#include <glm/vec4.hpp>

#include <string>

struct InterfaceTextureComponent {
    std::string texture;
    glm::vec4 colour;
};

#endif