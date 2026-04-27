#ifndef KIKI_RENDERER_DESCRIPTORS
#define KIKI_RENDERER_DESCRIPTORS

#include "VulkanWrapper.hpp"
#include "VulkanWindow.hpp"
#include "Image.hpp"

#include <cstdint>

namespace rutils {
    DescriptorSetLayout createSceneDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createObjectDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createMaterialDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createGBufferDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createCubemapDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createPostProcessingDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createSSAODescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createSSAOBlurredDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createTonemapDescriptorLayout(VulkanWindow const& window);
    DescriptorPool createDescriptorPool(VulkanWindow const& window, std::uint32_t aMaxDescriptors = 2048, std::uint32_t aMaxSets = 1024);
    DescriptorSetLayout createAnimationDescriptorLayout(VulkanWindow const& window);
    VkDescriptorSet allocDescSet(VulkanWindow const& window, VkDescriptorPool aPool, VkDescriptorSetLayout aSetLayout);
    void initialiseDeferredLightingDescriptorSet(VulkanWindow const& window, GBuffers& gbuffers, Image& depthBuffer, Sampler& sampler, VkDescriptorSet& deferredLightingDescriptors, Image& skyboxCubemap, Sampler& cubemapSampler);
    void initialisePostProcessingDescriptorSet(VulkanWindow const& window, GBuffers& gbuffers, Image& depthBuffer, Image& postProcessingImage, Sampler& sampler, VkDescriptorSet& postProcessingDescriptors);
    void initialiseSSAODescriptorSet(VulkanWindow const& window, GBuffers& gbuffers, Image& depthBuffer, Sampler& sampler, VkDescriptorSet& ssaoDescriptors);
    void initialiseSSAOHBlurDescriptorSet(VulkanWindow const& window, GBuffers& gbuffers, Image& depthBuffer, Sampler& sampler, VkDescriptorSet& ssaoHBlurDescriptors);
    void initialiseSSAOBlurredDescriptorSet(VulkanWindow const& window, GBuffers& gbuffers, Image& depthBuffer, Sampler& sampler, VkDescriptorSet& ssaoBlurredDescriptors);
    void initialiseTonemapDescriptorSet(VulkanWindow const& window, Image& doneSSRImage, Sampler& sampler, VkDescriptorSet& tonemapDescriptors);

    DescriptorSetLayout createInterfaceDescriptorLayout(VulkanWindow const& window);
    DescriptorSetLayout createInterfaceTextDescriptorLayout(VulkanWindow const& window);
}

#endif