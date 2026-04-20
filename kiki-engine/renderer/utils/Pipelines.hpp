#ifndef KIKI_RENDERER_PIPELINES
#define KIKI_RENDERER_PIPELINES

#include "VulkanWrapper.hpp"
#include "VulkanWindow.hpp"

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>

namespace rutils {
    struct ObjectData {
        glm::mat4 model;
        glm::vec4 baseColour;
        std::uint32_t sprite = 0;
        std::uint32_t padding[3];
    };

    struct Pipelines {
        rutils::Pipeline pbr;
        rutils::Pipeline pbr_alpha;
        rutils::Pipeline deferred_geometry;
        rutils::Pipeline deferred_geometry_alpha;
        rutils::Pipeline deferred_lighting;
        rutils::Pipeline fxaa;
        rutils::Pipeline ssr;
        rutils::Pipeline ssao;
    };

    struct PipelineLayouts {
        PipelineLayout pbrPipelineLayout;
        PipelineLayout deferredPipelineLayout;
        PipelineLayout skyboxPipelineLayout;
        PipelineLayout postprocessPipelineLayout;
    };

    Pipelines createAllPipelines(
        VulkanWindow const& window,
        PipelineLayouts const& pipelineLayouts
    );

    PipelineLayout createPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout materialLayout);
    PipelineLayout createPostProcessingPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout postProcessingLayout);
    Pipeline createPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    
    Pipeline createDeferredGeometryPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createDeferredGeometryAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createDeferredLightingPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createFXAAPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createSSRPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createSSAOPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
}

#endif