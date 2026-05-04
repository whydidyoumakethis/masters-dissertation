#include "TextureManager.hpp"

#include "FatalError.hpp"

#include <stb_image.h>

namespace Kiki {
    TextureManager& TextureManager::get() {
        static TextureManager instance;
        return instance;
    }

    void TextureManager::shutdown() {
        textures.clear();
    }

    std::string TextureManager::loadTexture(std::filesystem::path path, std::string name) {
        std::string key = name;

        stbi_set_flip_vertically_on_load(0);

        // Load base image
        int baseWidthi, baseHeighti, baseChannelsi;

        stbi_uc* data = stbi_load(path.string().c_str(), &baseWidthi, &baseHeighti, &baseChannelsi, 4 /* want 4 c h a n n e l s = RGBA */);

        if (!data) {
            throw Kiki::FatalError("{}: unable to load texture base image", stbi_failure_reason());
        }

        textures[key] = renderManager.loadInterfaceTexture(data, baseWidthi, baseHeighti);

        stbi_set_flip_vertically_on_load(1);
      
        return key;
    }

    iutils::InterfaceTexture& TextureManager::getTexture(std::string name) {
        return textures[name];
    }

    void TextureManager::deleteTexture(std::string name) {
        textures.erase(name);
    }

    void TextureManager::deleteAllTextures() {
        textures.clear();
    }
}