#ifndef KIKI_INTERFACE_TEXTUREMANAGER
#define KIKI_INTERFACE_TEXTUREMANAGER

#include "utils/InterfaceTexture.hpp"
#include "renderer/RenderManager.hpp"

#include <unordered_map>
#include <string>

namespace Kiki {
    class TextureManager {
        private:
        TextureManager() = default;
        ~TextureManager() = default;
        TextureManager(const TextureManager&) = delete;
        TextureManager& operator=(const TextureManager&) = delete;

        RenderManager& renderManager = RenderManager::get();

        std::unordered_map<std::string, iutils::InterfaceTexture> textures;

        public:
        static TextureManager& get();
        void shutdown();

        iutils::InterfaceTexture& getTexture(std::string name);
        std::string loadTexture(std::filesystem::path path, std::string name);
        void deleteTexture(std::string name);
        void deleteAllTextures();
    };
}

#endif