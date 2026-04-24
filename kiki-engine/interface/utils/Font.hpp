#ifndef KIKI_INTERFACE_FONT
#define KIKI_INTERFACE_FONT

#include "renderer/utils/Image.hpp"

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <volk.h>

#include <unordered_map>

namespace iutils {
    struct GlyphInfo {
        uint32_t charCode;
        float uMin, uMax, vMin, vMax;
        float width, height;
        float bearingX, bearingY;
        float advance;
    };

    class Font {
        public:
        std::unordered_map<uint32_t, GlyphInfo> glyphs;
        int baseSize;
        float lineHeight;
        float ascender;
        float descender;

        msdfgen::FontHandle* handle;

        int atlasSize = 2048;
        rutils::Image atlas;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        float currentX = 0, currentY = 0;
        float rowSize = 0;

        ~Font() {
            msdfgen::destroyFont(handle);
        }
    };
}

#endif