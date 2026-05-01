#ifndef KIKI_INTERFACE_FONTMANAGER
#define KIKI_INTERFACE_FONTMANAGER

#include "utils/Font.hpp"
#include "utils/LatinAlphabet.hpp"
#include "renderer/RenderManager.hpp"

#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include <unordered_map>
#include <filesystem>

namespace Kiki {
    struct StagingGlyph {
        uint32_t glyphId;
        msdfgen::Bitmap<float, 3> bitmap;
        double l, r, t, b;
    };

    enum class HorizontalAlignment {
        LEFT,
        CENTRE,
        RIGHT
    };

    enum class VerticalAlignment {
        TOP,
        CENTRE,
        BOTTOM
    };

    class FontManager {
        private:
        FontManager() = default;
        ~FontManager() = default;
        FontManager(const FontManager&) = delete;
        FontManager& operator=(const FontManager&) = delete;

        msdfgen::FreetypeHandle* freetype;
        FT_Library freetypeLibrary;
        RenderManager& renderManager = RenderManager::get();

        std::unordered_map<std::string, iutils::Font> fonts;

        public:
        static FontManager& get();
        void initialise();
        void shutdown();

        iutils::Font& getFont(std::string name);
        std::string loadFont(std::filesystem::path path, std::string name = "", std::u32string characters = LATIN_ALPHABET, int baseSize = 48);
        void deleteFont(std::string name);
        void deleteAllFonts();
    };
}

#endif