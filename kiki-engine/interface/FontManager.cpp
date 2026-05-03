#include "FontManager.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#include <hb-ft.h>
#include <tracy/Tracy.hpp>

#include <vector>


namespace Kiki {
    FontManager& FontManager::get() {
        static FontManager instance;
        return instance;
    }

    void FontManager::initialise() {
        freetype = msdfgen::initializeFreetype();
        FT_Init_FreeType(&freetypeLibrary);
    }

    void FontManager::shutdown() {
        fonts.clear();
        msdfgen::deinitializeFreetype(freetype);
    }

    std::string FontManager::loadFont(std::filesystem::path path, std::string name, std::u32string characters, int baseSize) {
        std::string key;

        if (freetype) {
            iutils::Font font;
            font.msdfHandle = msdfgen::loadFont(freetype, path.string().c_str());
            FT_New_Face(freetypeLibrary, path.string().c_str(), 0, &font.freetypeFace);
            FT_Select_Charmap(font.freetypeFace, FT_ENCODING_UNICODE);

            if (font.msdfHandle) {
                font.baseSize = baseSize;
                hb_face_t* hbFace = hb_ft_face_create_referenced(font.freetypeFace);
                font.hbHandle = hb_font_create(hbFace);

                msdfgen::FontMetrics metrics;
                msdfgen::getFontMetrics(metrics, font.msdfHandle);
                double scale = baseSize / metrics.emSize;

                font.ascender = metrics.ascenderY * scale;
                font.descender = metrics.descenderY * scale;
                font.lineHeight = metrics.lineHeight * scale;
                font.emSize = metrics.emSize;

                hb_font_set_scale(font.hbHandle, font.baseSize << 6, font.baseSize << 6);

                std::vector<stbrp_rect> packingRects(characters.size());
                std::vector<StagingGlyph> tempGlyphs(characters.size());

                for (int i = 0; i <  characters.size(); i++) {
                    tempGlyphs[i].glyphId = FT_Get_Char_Index(font.freetypeFace, (FT_ULong)characters[i]); 
                    
                    msdfgen::Shape shape;
                    if (!msdfgen::loadGlyph(shape, font.msdfHandle, characters[i]))
                        continue;

                    shape.normalize();
                    msdfgen::edgeColoringSimple(shape, 3.0);
                    msdfgen::Shape::Bounds bounds = shape.getBounds();
                    int w = (int) std::ceil((bounds.r - bounds.l) * scale) + 8;
                    int h = (int) std::ceil((bounds.t - bounds.b) * scale) + 8;

                    packingRects[i].id = i;
                    packingRects[i].w = w;
                    packingRects[i].h = h;

                    tempGlyphs[i].l = bounds.l * scale;
                    tempGlyphs[i].b = bounds.b * scale;
                    tempGlyphs[i].r = bounds.r * scale;
                    tempGlyphs[i].t = bounds.t * scale;

                    tempGlyphs[i].bitmap = msdfgen::Bitmap<float, 3>(w, h);

                    msdfgen::Projection projection(
                        msdfgen::Vector2(scale, scale),
                        msdfgen::Vector2(-bounds.l + 4.0 / scale, -bounds.b + 4.0 / scale)
                    );
                    msdfgen::Range range(4.0 / scale);

                    msdfgen::MSDFGeneratorConfig config;
                    // This tells the generator: "If a pixel looks like it will create a hole, fix it."
                    config.errorCorrection.mode = msdfgen::ErrorCorrectionConfig::EDGE_PRIORITY;
                    config.errorCorrection.distanceCheckMode = msdfgen::ErrorCorrectionConfig::DistanceCheckMode::ALWAYS_CHECK_DISTANCE;
                    config.errorCorrection.mode = msdfgen::ErrorCorrectionConfig::Mode::INDISCRIMINATE;
                    msdfgen::generateMSDF(tempGlyphs[i].bitmap, shape, msdfgen::SDFTransformation(projection, range), config);
                }

                stbrp_context context;
                std::vector<stbrp_node> nodes(font.atlasSize);
                stbrp_init_target(&context, font.atlasSize, font.atlasSize, nodes.data(), nodes.size());
                stbrp_pack_rects(&context, packingRects.data(), packingRects.size());

                std::vector<uint8_t> atlas(font.atlasSize * font.atlasSize * 4, 0);

                for (int i = 0; i < packingRects.size(); i++) {
                    if (!packingRects[i].was_packed)
                        continue;

                    for (int x = 0; x < tempGlyphs[i].bitmap.width(); x++) {
                        for (int y = 0; y < tempGlyphs[i].bitmap.height(); y++) {
                            auto pixel = tempGlyphs[i].bitmap(x, y);
                            int index = ((packingRects[i].x + x) + (packingRects[i].y + y) * font.atlasSize) * 4;

                            atlas[index] = (uint8_t) std::clamp(pixel[0] * 255.0f, 0.0f, 255.0f);
                            atlas[index + 1] = (uint8_t) std::clamp(pixel[1] * 255.0f, 0.0f, 255.0f);
                            atlas[index + 2] = (uint8_t) std::clamp(pixel[2] * 255.0f, 0.0f, 255.0f);
                            atlas[index + 3] = (uint8_t) 1.0f;
                        }
                    }

                    iutils::GlyphInfo info;
                    info.glyphId = tempGlyphs[i].glyphId;
                    info.uMin = ((float) packingRects[i].x + 0.5f) / font.atlasSize;
                    info.uMax = ((float) (packingRects[i].x + packingRects[i].w) - 0.5f) / font.atlasSize;
                    info.vMin = ((float) packingRects[i].y + 0.5f) / font.atlasSize;
                    info.vMax = ((float) (packingRects[i].y + packingRects[i].h) - 0.5f) / font.atlasSize;
                    info.width = packingRects[i].w;
                    info.height = packingRects[i].h;
                    info.l = tempGlyphs[i].l;
                    info.r = tempGlyphs[i].r;
                    info.t = tempGlyphs[i].t;
                    info.b = tempGlyphs[i].b;

                    std::vector<float> positions = {
                        0.0f, 0.0f, info.uMin, info.vMin,
                        info.width, 0.0f, info.uMax, info.vMin,
                        info.width, -info.height, info.uMax, info.vMax,
                        0.0f, -info.height, info.uMin, info.vMax
                    };

                    info.vertices = renderManager.updateInterfaceVertices(positions);

                    font.glyphs[tempGlyphs[i].glyphId] = std::move(info);
                }

                renderManager.loadFontAtlas(&font, atlas);

                key = name;
                fonts.emplace(key, std::move(font));
            }
        }
      
        return key;
    }

    iutils::Font& FontManager::getFont(std::string name) {
        return fonts[name];
    }

    void FontManager::deleteFont(std::string name) {
        fonts.erase(name);
    }

    void FontManager::deleteAllFonts() {
        fonts.clear();
    }
}