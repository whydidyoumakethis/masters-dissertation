#ifndef KIKI_INTERFACE_TEXTUREMANAGER
#define KIKI_INTERFACE_TEXTUREMANAGER

namespace Kiki {
    class TextureManager {
        private:
        TextureManager() = default;
        ~TextureManager() = default;
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

        public:
        static TextureManager& get();
    };
}

#endif