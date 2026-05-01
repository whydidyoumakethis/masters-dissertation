#ifndef KIKI_COMPONENTS_TEXTCOMPONENT
#define KIKI_COMPONENTS_TEXTCOMPONENT

#include "renderer/utils/Buffer.hpp"
#include "interface/FontManager.hpp"

#include <vector>
#include <string>

struct CharacterTransform {
    rutils::Buffer* buffer;
    glm::mat4 transform;
};

struct TextComponent {
    std::string font;
    std::u32string text;
    float size;
    glm::vec3 colour;
    float transparency;
    Kiki::HorizontalAlignment horizontalAlignment = Kiki::HorizontalAlignment::CENTRE;
    Kiki::VerticalAlignment verticalAlignment = Kiki::VerticalAlignment::CENTRE;

    bool dirty = true;
    std::vector<CharacterTransform> characters;
    TextComponent() = default;
    TextComponent(TextComponent const&) = delete;
    TextComponent& operator= (TextComponent const&) = delete;

    TextComponent(TextComponent&&) noexcept = default;
    TextComponent& operator=(TextComponent&&) noexcept = default;

    TextComponent(std::string name, std::u32string text, float size, glm::vec3 colour, float transparency, 
        Kiki::HorizontalAlignment horizontalAlignment = Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment verticalAlignment = Kiki::VerticalAlignment::CENTRE)
        : font(name), text(text), size(size), colour(colour), transparency(transparency), horizontalAlignment(horizontalAlignment), verticalAlignment(verticalAlignment) {}
};

#endif