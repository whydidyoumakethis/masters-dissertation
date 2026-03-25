#ifndef KIKI_RENDERER_DESCRIPTORS
#define KIKI_RENDERER_DESCRIPTORS

#include "VulkanWrapper.hpp"
#include "VulkanWindow.hpp"

#include <cstdint>

namespace rutils {
    DescriptorSetLayout createSceneDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createObjectDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createMaterialDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createGBufferDescriptorLayout(VulkanWindow const& window);
    DescriptorPool createDescriptorPool(VulkanWindow const& window, std::uint32_t aMaxDescriptors = 2048, std::uint32_t aMaxSets = 1024);
    VkDescriptorSet allocDescSet(VulkanWindow const& window, VkDescriptorPool aPool, VkDescriptorSetLayout aSetLayout);
}

#endif