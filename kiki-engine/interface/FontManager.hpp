#ifndef KIKI_INTERFACE_FONTMANAGER
#define KIKI_INTERFACE_FONTMANAGER

#include "utils/Font.hpp"

#include <msdfgen.h>
#include <msdfgen-ext.h>

#include <unordered_map>
#include <filesystem>

namespace Kiki {
    struct StagingGlyph {
        msdfgen::Bitmap<float, 3> bitmap;
        double l, r, t, b;
        double advance;
    };

    class FontManager {
        private:
        FontManager() = default;
        ~FontManager() = default;
        FontManager(const FontManager&) = delete;
        FontManager& operator=(const FontManager&) = delete;

        msdfgen::FreetypeHandle* freetype;

        std::unordered_map<std::string, iutils::Font> fonts;

        public:
        static FontManager& get();
        void initialise();
        void shutdown();

        std::string loadFont(std::filesystem::path path, std::string name = "", std::u32string characters = U"", int baseSize = 48);
        void addCharacters(iutils::Font* font, std::u32string characters);
        void deleteFont(std::string name);
        void deleteAllFonts();
    };
}

#endif