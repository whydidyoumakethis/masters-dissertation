#ifndef KIKI_INTERFACE_INTERFACETEXTURE
#define KIKI_INTERFACE_INTERFACETEXTURE

#include "renderer/utils/Image.hpp"

#include <volk.h>

namespace iutils {
    class InterfaceTexture {
        public:
        rutils::Image image;
        VkDescriptorSet descriptorSet;
    };
}

#endif