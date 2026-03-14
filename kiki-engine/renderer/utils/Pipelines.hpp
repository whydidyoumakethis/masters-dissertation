#ifndef KIKI_RENDERER_PIPELINES
#define KIKI_RENDERER_PIPELINES

#include "VulkanWrapper.hpp"
#include "VulkanWindow.hpp"

namespace rutils {
    PipelineLayout createPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout objectLayout);
    Pipeline createPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
}

#endif