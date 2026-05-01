#include "InterfaceSystem.hpp"

#include <tracy/Tracy.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <hb.h>

namespace Kiki {
    void InterfaceSystem::OnStart() {
        fontManager.initialise();
    }

    // Update position of InterfaceComponents and fire any events
    void InterfaceSystem::OnUpdate(float dt) {
        {
            ZoneScopedN("Sort interface components");
            registry.sort<InterfaceComponent>([](const auto& lhs, const auto& rhs) {
                return lhs.zindex < rhs.zindex;
                });
        }

        {
            ZoneScopedN("Interface animations");

            auto animComponents = world.Query<InterfaceAnimationComponent>();

            for (auto [e, animationComponent] : animComponents.each()) {
                if (registry.all_of<InterfaceComponent>(e)) {
                    auto& interfaceComponent = registry.get<InterfaceComponent>(e);

                    if (!animationComponent.init) {
                        animationComponent.positionDiff = ScaleVec2D(animationComponent.targetPosition.scaleX - interfaceComponent.position.scaleX,
                                                                     animationComponent.targetPosition.x - interfaceComponent.position.x,
                                                                     animationComponent.targetPosition.scaleY - interfaceComponent.position.scaleY,
                                                                     animationComponent.targetPosition.y - interfaceComponent.position.y);
                        animationComponent.sizeDiff = ScaleVec2D(animationComponent.targetSize.scaleX - interfaceComponent.size.scaleX,
                                                                 animationComponent.targetSize.x - interfaceComponent.size.x,
                                                                 animationComponent.targetSize.scaleY - interfaceComponent.size.scaleY,
                                                                 animationComponent.targetSize.y - interfaceComponent.size.y);
                        animationComponent.rotationDiff = animationComponent.targetRotation - interfaceComponent.rotation;

                        if (registry.all_of<BackgroundComponent>(e)) {
                            auto& backgroundComponent = registry.get<BackgroundComponent>(e);

                            animationComponent.backgroundColourDiff = animationComponent.targetBackgroundColour - backgroundComponent.colour;
                            animationComponent.backgroundTransparencyDiff = animationComponent.targetBackgroundTransparency - backgroundComponent.transparency;
                        }

                        if (registry.all_of<TextComponent>(e)) {
                            auto& textComponent = registry.get<TextComponent>(e);

                            animationComponent.textColourDiff = animationComponent.targetTextColour - textComponent.colour;
                            animationComponent.textTransparencyDiff = animationComponent.targetTextTransparency - textComponent.transparency;
                            animationComponent.textSizeDiff = animationComponent.targetTextSize - textComponent.size;
                        }

                        animationComponent.init = true;
                    }

                    float prevT = std::min(1.0f, animationComponent.elapsed / animationComponent.time);
                    animationComponent.elapsed += dt;
                    float t = std::min(1.0f, animationComponent.elapsed / animationComponent.time);

                    switch (animationComponent.interpolation) {
                    case InterfaceInterpolationType::LINEAR:
                        break;
                    case InterfaceInterpolationType::EASE_IN:
                        t = t * t;
                        prevT = prevT * prevT;
                        break;
                    case InterfaceInterpolationType::EASE_OUT:
                        t = 2.0f * t - t * t;
                        prevT = 2.0f * prevT - prevT * prevT;
                        break;
                    case InterfaceInterpolationType::EASE_IN_OUT:
                        if (t < 0.5) {
                            t = 2.0f * t * t;
                        } else {
                            t = -1.0f + (4.0f - 2.0f * t) * t;
                        }

                        if (prevT < 0.5) {
                            prevT = 2.0f * prevT * prevT;
                        } else {
                            prevT = -1.0f + (4.0f - 2.0f * prevT) * prevT;
                        }
                        break;
                    }

                    if (animationComponent.reversing) {
                        t = 1.0f - t;
                        prevT = 1.0f - prevT;
                    }

                    interfaceComponent.position = ScaleVec2D(interfaceComponent.position.scaleX - (prevT * animationComponent.positionDiff.scaleX) + (t * animationComponent.positionDiff.scaleX),
                                                             interfaceComponent.position.x - (prevT * animationComponent.positionDiff.x) + (t * animationComponent.positionDiff.x),
                                                             interfaceComponent.position.scaleY - (prevT * animationComponent.positionDiff.scaleY) + (t * animationComponent.positionDiff.scaleY),
                                                             interfaceComponent.position.y - (prevT * animationComponent.positionDiff.y) + (t * animationComponent.positionDiff.y));
                    interfaceComponent.size = ScaleVec2D(interfaceComponent.size.scaleX - (prevT * animationComponent.sizeDiff.scaleX) + (t * animationComponent.sizeDiff.scaleX),
                                                             interfaceComponent.size.x - (prevT * animationComponent.sizeDiff.x) + (t * animationComponent.sizeDiff.x),
                                                             interfaceComponent.size.scaleY - (prevT * animationComponent.sizeDiff.scaleY) + (t * animationComponent.sizeDiff.scaleY),
                                                             interfaceComponent.size.y - (prevT * animationComponent.sizeDiff.y) + (t * animationComponent.sizeDiff.y));
                    interfaceComponent.rotation = interfaceComponent.rotation - (prevT * animationComponent.rotationDiff) + (t * animationComponent.rotationDiff);
                    interfaceComponent.dirty = true;

                    if (registry.all_of<BackgroundComponent>(e)) {
                            auto& backgroundComponent = registry.get<BackgroundComponent>(e);

                            backgroundComponent.colour = backgroundComponent.colour - (prevT * animationComponent.backgroundColourDiff) + (t * animationComponent.backgroundColourDiff);
                            backgroundComponent.transparency = backgroundComponent.transparency - (prevT * animationComponent.backgroundTransparencyDiff) + (t * animationComponent.backgroundTransparencyDiff);
                        }

                        if (registry.all_of<TextComponent>(e)) {
                            auto& textComponent = registry.get<TextComponent>(e);

                            textComponent.colour = textComponent.colour - (prevT * animationComponent.textColourDiff) + (t * animationComponent.textColourDiff);
                            textComponent.transparency = textComponent.transparency - (prevT * animationComponent.textTransparencyDiff) + (t * animationComponent.textTransparencyDiff);
                            textComponent.size = textComponent.size - (prevT * animationComponent.textSizeDiff) + (t * animationComponent.textSizeDiff);

                            textComponent.dirty = true;
                        }

                    if (animationComponent.elapsed >= animationComponent.time) {
                        if (animationComponent.loop && !animationComponent.reverse) {
                            animationComponent.elapsed = 0.0f;
                        } else if (animationComponent.loop && animationComponent.reverse) {
                            animationComponent.elapsed = 0.0f;
                            animationComponent.reversing = !animationComponent.reversing;
                        } else if (!animationComponent.loop && animationComponent.reverse && !animationComponent.reversing) {
                            animationComponent.elapsed = 0.0f;
                            animationComponent.reversing = true;
                        } else {
                            registry.remove<InterfaceAnimationComponent>(e);
                        }
                    }
                }
            }
        }

        auto uiComponents = world.Query<InterfaceComponent>();
        WindowExtent extent = renderManager.getWindowExtent();

        {
            ZoneScopedN("Updating interface positions, sizes and button states");

            for (auto [e, interfaceComponent] : uiComponents.each()) {
                if (interfaceComponent.dirty) {
                    float cursorX = 0.0f;
                    float cursorY = 0.0f;
                    float width = (float)extent.width;
                    float height = (float)extent.height;

                    if (interfaceComponent.parent != entt::null && registry.all_of<InterfaceComponent>(interfaceComponent.parent)) {
                        auto& parentComp = registry.get<InterfaceComponent>(interfaceComponent.parent);

                        if (parentComp.dirty)
                            continue;

                        width = parentComp.size.absoluteX;
                        height = parentComp.size.absoluteY;
                        cursorX = parentComp.position.absoluteX;
                        cursorY = parentComp.position.absoluteY;
                    }

                    interfaceComponent.position.absoluteX = (width * interfaceComponent.position.scaleX) + interfaceComponent.position.x + cursorX;
                    interfaceComponent.position.absoluteY = (height * interfaceComponent.position.scaleY) + interfaceComponent.position.y + cursorY;

                    interfaceComponent.size.absoluteX = (width * interfaceComponent.size.scaleX) + interfaceComponent.size.x;
                    interfaceComponent.size.absoluteY = (height * interfaceComponent.size.scaleY) + interfaceComponent.size.y;

                    if (registry.all_of<AspectRatioComponent>(e)) {
                        auto& aspectRatioComp = registry.get<AspectRatioComponent>(e);

                        float aspectRatio = interfaceComponent.size.absoluteX / interfaceComponent.size.absoluteY;

                        if (aspectRatio > aspectRatioComp.aspectRatio) {
                            interfaceComponent.size.absoluteX = aspectRatioComp.aspectRatio * interfaceComponent.size.absoluteY;
                        } else if (aspectRatio < aspectRatioComp.aspectRatio) {
                            interfaceComponent.size.absoluteY = interfaceComponent.size.absoluteX / aspectRatioComp.aspectRatio;
                        }
                    }

                    glm::mat4 model = glm::mat4(1.0f);

                    model = glm::translate(model, glm::vec3(interfaceComponent.position.absoluteX, interfaceComponent.position.absoluteY, 0.0f));

                    model = glm::translate(model, glm::vec3(0.5f * interfaceComponent.size.absoluteX, 0.5f * interfaceComponent.size.absoluteY, 0.0f));
                    model = glm::rotate(model, glm::radians(interfaceComponent.rotation), glm::vec3(0.0f, 0.0f, 1.0f));
                    model = glm::translate(model, glm::vec3(-0.5f * interfaceComponent.size.absoluteX, -0.5f * interfaceComponent.size.absoluteY, 0.0f));

                    model = glm::scale(model, glm::vec3(interfaceComponent.size.absoluteX, interfaceComponent.size.absoluteY, 1.0f));

                    interfaceComponent.model = model;

                    if (registry.all_of<TextComponent>(e)) {
                        auto& textComponent = registry.get<TextComponent>(e);
                        textComponent.dirty = true;
                    }

                    interfaceComponent.dirty = false;
                }

                if (registry.all_of<ButtonComponent>(e)) {
                    auto& buttonComponent = registry.get<ButtonComponent>(e);
                    bool mouseWithin = false;
                    bool background = registry.all_of<BackgroundComponent>(e);
                    float x, y;
                    inputManager.getMousePosition(x, y);

                    if (x >= interfaceComponent.position.absoluteX && x <= interfaceComponent.position.absoluteX + interfaceComponent.size.absoluteX &&
                        y >= interfaceComponent.position.absoluteY && y <= interfaceComponent.position.absoluteY + interfaceComponent.size.absoluteY) {
                        mouseWithin = true;
                    }

                    if (mouseWithin && inputManager.isMouseButtonJustUp(GLFW_MOUSE_BUTTON_1) && buttonComponent.buttonState == ButtonState::BUTTON_DOWN) {
                        messageCenter.Publish(ButtonClickEvent(e, ClickType::BUTTON_RELEASE));
                        buttonComponent.buttonState = ButtonState::HOVER;

                        if (background) {
                            auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                            backgroundComponent.colour = glm::vec3(buttonComponent.hoverColour.x, buttonComponent.hoverColour.y, buttonComponent.hoverColour.z);
                            backgroundComponent.transparency = 1.0f - buttonComponent.hoverColour.w;
                        }
                    }
                    else if (mouseWithin && inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_1)) {
                        messageCenter.Publish(ButtonClickEvent(e, ClickType::BUTTON_DOWN));
                        buttonComponent.buttonState = ButtonState::BUTTON_DOWN;

                        if (background) {
                            auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                            backgroundComponent.colour = glm::vec3(buttonComponent.clickColour.x, buttonComponent.clickColour.y, buttonComponent.clickColour.z);
                            backgroundComponent.transparency = 1.0f - buttonComponent.clickColour.w;
                        }
                    }
                    else if (mouseWithin && buttonComponent.buttonState != ButtonState::HOVER && buttonComponent.buttonState != ButtonState::BUTTON_DOWN) {
                        messageCenter.Publish(ButtonHoverEvent(e));
                        buttonComponent.buttonState = ButtonState::HOVER;

                        if (background) {
                            auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                            backgroundComponent.colour = glm::vec3(buttonComponent.hoverColour.x, buttonComponent.hoverColour.y, buttonComponent.hoverColour.z);
                            backgroundComponent.transparency = 1.0f - buttonComponent.hoverColour.w;
                        }
                    }
                    else if (!mouseWithin && buttonComponent.buttonState != ButtonState::NONE) {
                        buttonComponent.buttonState = ButtonState::NONE;

                        if (background) {
                            auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                            backgroundComponent.colour = glm::vec3(buttonComponent.colour.x, buttonComponent.colour.y, buttonComponent.colour.z);
                            backgroundComponent.transparency = 1.0f - buttonComponent.colour.w;
                        }
                    }
                }
            }
        }

        auto textComponents = world.Query<TextComponent>();

        {
            ZoneScopedN("Updating text components");

            for (auto [e, textComponent] : textComponents.each()) {
                if (textComponent.dirty && registry.all_of<InterfaceComponent>(e)) {
                    auto& interfaceComponent = registry.get<InterfaceComponent>(e);
                    auto& font = fontManager.getFont(textComponent.font);

                    textComponent.characters.clear();

                    hb_buffer_t* buffer = hb_buffer_create();
                    hb_buffer_add_utf32(
                        buffer,
                        (const uint32_t*)textComponent.text.data(),
                        textComponent.text.length(),
                        0,
                        textComponent.text.length()
                    );
                    hb_buffer_guess_segment_properties(buffer);
                    hb_shape(font.hbHandle, buffer, nullptr, 0);

                    float scale = (float)textComponent.size / (float)font.baseSize;

                    unsigned int numGlyphs;
                    hb_glyph_info_t* glyphInfo = hb_buffer_get_glyph_infos(buffer, &numGlyphs);
                    hb_glyph_position_t* glyphPosition = hb_buffer_get_glyph_positions(buffer, &numGlyphs);

                    float width = 0.0f;
                    float maxTop = 0.0f;
                    float minBottom = 0.0f;

                    for (int i = 0; i < numGlyphs; i++) {
                        width += (glyphPosition[i].x_advance / 64.0f) * scale;

                        iutils::GlyphInfo& msdfInfo = font.glyphs[glyphInfo[i].codepoint];
                        float top = msdfInfo.t * scale;
                        float bottom = msdfInfo.b * scale;

                        if (top > maxTop) {
                            maxTop = top;
                        }

                        if (bottom < minBottom) {
                            minBottom = bottom;
                        }
                    }

                    float cursorX = 0.0f;
                    switch (textComponent.horizontalAlignment) {
                    case HorizontalAlignment::LEFT:
                        cursorX = interfaceComponent.position.absoluteX;
                        break;
                    case HorizontalAlignment::CENTRE:
                        cursorX = interfaceComponent.position.absoluteX + (interfaceComponent.size.absoluteX - width) * 0.5f;
                        break;
                    case HorizontalAlignment::RIGHT:
                        cursorX = interfaceComponent.position.absoluteX + interfaceComponent.size.absoluteX - width;
                        break;
                    }

                    float cursorY = 0.0f;
                    switch (textComponent.verticalAlignment) {
                    case VerticalAlignment::TOP:
                        cursorY = interfaceComponent.position.absoluteY + (font.ascender * scale);
                        break;
                    case VerticalAlignment::CENTRE:
                        cursorY = interfaceComponent.position.absoluteY + ((interfaceComponent.size.absoluteY - (font.ascender - font.descender) * scale) * 0.5f) + font.ascender * scale;
                        break;
                    case VerticalAlignment::BOTTOM:
                        cursorY = interfaceComponent.position.absoluteY + interfaceComponent.size.absoluteY - (font.descender * scale);
                        break;
                    }

                    for (int i = 0; i < numGlyphs; i++) {
                        uint32_t gid = glyphInfo[i].codepoint;

                        if (font.glyphs.contains(gid)) {
                            glm::mat4 model = glm::mat4(1.0f);
                            iutils::GlyphInfo& msdfInfo = font.glyphs[gid];

                            float offsetX = glyphPosition[i].x_offset * scale;
                            float offsetY = glyphPosition[i].y_offset * scale;

                            model = glm::translate(model, glm::vec3(cursorX + offsetX + msdfInfo.l * scale, cursorY - offsetY - msdfInfo.b * scale, 0.0f));
                            model = glm::scale(model, glm::vec3(scale, scale, 1.0f));

                            textComponent.characters.push_back(CharacterTransform(&msdfInfo.vertices, model));
                        }

                        cursorX += (glyphPosition[i].x_advance / 64.0f) * scale;
                        cursorY += (glyphPosition[i].y_advance / 64.0f) * scale;
                    }

                    hb_buffer_destroy(buffer);

                    textComponent.dirty = false;
                }
            }
        }
    }

    void InterfaceSystem::OnStop() {
        fontManager.shutdown();
    }
} 
