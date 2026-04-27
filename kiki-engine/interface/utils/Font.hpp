#ifndef KIKI_INTERFACE_FONT
#define KIKI_INTERFACE_FONT

#include "renderer/utils/Image.hpp"

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <hb.h>
#include <volk.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>

namespace iutils {
    struct GlyphInfo {
        uint32_t glyphId;
        float uMin, uMax, vMin, vMax;
        float width, height;
        float l, r, t, b;
    };

    class Font {
        public:
        std::unordered_map<uint32_t, GlyphInfo> glyphs;
        int baseSize;
        float lineHeight;
        float ascender;
        float descender;
        float emSize;

        FT_Face freetypeFace;
        hb_font_t* hbHandle;
        msdfgen::FontHandle* msdfHandle;

        int atlasSize = 2048;
        rutils::Image atlas;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

        float currentX = 0, currentY = 0;
        float rowSize = 0;

        ~Font() = default;

        Font(Font&& other) noexcept = default;
        Font& operator=(Font&& other) noexcept = default;

        Font() = default;

        Font(const Font&) = delete;
        Font& operator=(const Font&) = delete;
    };
}

#endif