#include "InterfaceSystem.hpp"

#include "Components/InterfaceComponent.hpp"
#include "Components/BackgroundComponent.hpp"
#include "Components/TextComponent.hpp"
#include "Components/ButtonComponent.hpp"
#include "events/ButtonClickEvent.hpp"
#include "events/ButtonHoverEvent.hpp"

#include <hb.h>

namespace Kiki {
    void InterfaceSystem::OnStart() {
        fontManager.initialise();
    }

    // Update position of InterfaceComponents and fire any events
    void InterfaceSystem::OnUpdate(float dt) {
        registry.sort<InterfaceComponent>([](const auto& lhs, const auto& rhs) {
            return lhs.zindex < rhs.zindex;
        });

        auto uiComponents = world.Query<InterfaceComponent>();
        WindowExtent extent = renderManager.getWindowExtent();

        for (auto [e, interfaceComponent] : uiComponents.each()) {
            if (interfaceComponent.dirty) {
                float cursorX = 0.0f;
                float cursorY = 0.0f;
                float width = (float) extent.width;
                float height = (float) extent.height;

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

                if (registry.all_of<BackgroundComponent>(e)) {
                    auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                    std::vector<float> positions = {
                        interfaceComponent.position.absoluteX, interfaceComponent.position.absoluteY, 0.0f, 0.0f,
                        interfaceComponent.position.absoluteX + interfaceComponent.size.absoluteX, interfaceComponent.position.absoluteY, 1.0f, 0.0f,
                        interfaceComponent.position.absoluteX + interfaceComponent.size.absoluteX, interfaceComponent.position.absoluteY + interfaceComponent.size.absoluteY, 1.0f, 1.0f,
                        interfaceComponent.position.absoluteX, interfaceComponent.position.absoluteY + interfaceComponent.size.absoluteY, 0.0f, 1.0f
                    };

                    backgroundComponent.vertices = renderManager.updateInterfaceVertices(positions);
                }

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
                } else if (mouseWithin && inputManager.isMouseButtonJustDown(GLFW_MOUSE_BUTTON_1)) {
                    messageCenter.Publish(ButtonClickEvent(e, ClickType::BUTTON_DOWN));
                    buttonComponent.buttonState = ButtonState::BUTTON_DOWN;

                    if (background) {
                        auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                        backgroundComponent.colour = glm::vec3(buttonComponent.clickColour.x, buttonComponent.clickColour.y, buttonComponent.clickColour.z);
                        backgroundComponent.transparency = 1.0f - buttonComponent.clickColour.w;
                    }
                } else if (mouseWithin && buttonComponent.buttonState != ButtonState::HOVER && buttonComponent.buttonState != ButtonState::BUTTON_DOWN) {
                    messageCenter.Publish(ButtonHoverEvent(e));
                    buttonComponent.buttonState = ButtonState::HOVER;

                    if (background) {
                        auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                        backgroundComponent.colour = glm::vec3(buttonComponent.hoverColour.x, buttonComponent.hoverColour.y, buttonComponent.hoverColour.z);
                        backgroundComponent.transparency = 1.0f - buttonComponent.hoverColour.w;
                    }
                } else if (!mouseWithin) {
                    buttonComponent.buttonState = ButtonState::NONE;

                    if (background) {
                        auto& backgroundComponent = registry.get<BackgroundComponent>(e);
                        backgroundComponent.colour = glm::vec3(buttonComponent.colour.x, buttonComponent.colour.y, buttonComponent.colour.z);
                        backgroundComponent.transparency = 1.0f - buttonComponent.colour.w;
                    }
                }
            }
        }

        auto textComponents = world.Query<TextComponent>();

        for (auto [e, textComponent] : textComponents.each()) {
            if (textComponent.dirty && registry.all_of<InterfaceComponent>(e)) {
                auto& interfaceComponent = registry.get<InterfaceComponent>(e);
                auto& font = fontManager.getFont(textComponent.font);

                textComponent.vertices.clear();

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

                    iutils::GlyphInfo msdfInfo = font.glyphs[glyphInfo[i].codepoint];
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
                        iutils::GlyphInfo msdfInfo = font.glyphs[gid];

                        float offsetX = glyphPosition[i].x_offset * scale;
                        float offsetY = glyphPosition[i].y_offset * scale;

                        float x0 = cursorX + offsetX + (msdfInfo.l * scale);
                        float y0 = cursorY - offsetY - (msdfInfo.t * scale);
                        float x1 = x0 + (msdfInfo.width * scale);
                        float y1 = y0 + (msdfInfo.height * scale);

                        std::vector<float> positions = {
                            x0, y0, msdfInfo.uMin, msdfInfo.vMax,
                            x1, y0, msdfInfo.uMax, msdfInfo.vMax,
                            x1, y1, msdfInfo.uMax, msdfInfo.vMin,
                            x0, y1, msdfInfo.uMin, msdfInfo.vMin
                        };

                        textComponent.vertices.push_back(std::move(renderManager.updateInterfaceVertices(positions)));
                    }

                    cursorX += (glyphPosition[i].x_advance / 64.0f) * scale;
                    cursorY += (glyphPosition[i].y_advance / 64.0f) * scale;
                }

                hb_buffer_destroy(buffer);

                textComponent.dirty = false;
            }
        }
    }

    void InterfaceSystem::OnStop() {
        fontManager.shutdown();
    }
} 
