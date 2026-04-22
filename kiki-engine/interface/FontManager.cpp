#include "FontManager.hpp"

#define STB_RECT_PACK_IMPLEMENTATION
#include <stb_rect_pack.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <vector>

namespace Kiki {
    FontManager& FontManager::get() {
        static FontManager instance;
        return instance;
    }

    void FontManager::initialise() {
        freetype = msdfgen::initializeFreetype();
    }

    void FontManager::shutdown() {
        fonts.clear();
        msdfgen::deinitializeFreetype(freetype);
    }

    std::string FontManager::loadFont(std::filesystem::path path, std::string name, std::u32string characters, int baseSize) {
        if (freetype) {
            msdfgen::FontHandle* handle = msdfgen::loadFont(freetype, path.string().c_str());

            if (handle) {
                iutils::Font font;
                font.handle = handle;
                font.baseSize = baseSize;
                FT_Face face = (FT_Face) font.handle;

                msdfgen::FontMetrics metrics;
                msdfgen::getFontMetrics(metrics, font.handle);
                double scale = (double) baseSize / metrics.emSize;

                font.ascender = (float) (metrics.ascenderY * scale);
                font.descender = (float) (metrics.descenderY * scale);
                font.lineHeight = (float) (metrics.lineHeight * scale);

                std::vector<stbrp_rect> packingRects(characters.size());
                std::vector<StagingGlyph> tempGlyphs(characters.size());

                for (int i = 0; i <  characters.size(); i++) {
                    msdfgen::Shape shape;
                    if (!msdfgen::loadGlyph(shape, font.handle, characters[i]))
                        continue;

                    shape.normalize();
                    msdfgen::edgeColoringSimple(shape, 3.0);
                    msdfgen::Shape::Bounds bounds = shape.getBounds();
                    int w = (int) std::ceil((bounds.r - bounds.l) * scale) + 4;
                    int h = (int) std::ceil((bounds.t - bounds.b) * scale) + 4;

                    packingRects[i].id = i;
                    packingRects[i].w = w;
                    packingRects[i].h = h;

                    tempGlyphs[i].l = bounds.l;
                    tempGlyphs[i].b = bounds.b; 
                    tempGlyphs[i].r = bounds.r;
                    tempGlyphs[i].t = bounds.t;
                }
            }
        }
    }

    void FontManager::addCharacters(iutils::Font* font, std::u32string characters) {

    }

    void FontManager::deleteFont(std::string name) {
        fonts.erase(name);
    }

    void FontManager::deleteAllFonts() {
        fonts.clear();
    }
}