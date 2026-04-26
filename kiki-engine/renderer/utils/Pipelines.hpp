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
        // std::uint32_t sprite = 0;
        // std::uint32_t useTexture = 1;
        glm::vec4 flags;
    };

    struct SSRSettings {
        glm::vec4 settings = glm::vec4(64, 6, 0.25f, 0.4f);
        // x = maxSteps, int
        // y = binarySteps, int
        // z = stepSize, float
        // w = thicknessTolerance, float
    };

    struct SSAOSettings {
        std::uint32_t width;
        std::uint32_t height;
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
        rutils::Pipeline ssao_hblur;
        rutils::Pipeline ssao_blurred;
    };

    struct PipelineLayouts {
        PipelineLayout pbrPipelineLayout;
        PipelineLayout deferredPipelineLayout;
        PipelineLayout skyboxPipelineLayout;
        PipelineLayout postprocessPipelineLayout;
        PipelineLayout ssaoPipelineLayout;
        PipelineLayout ssaoBlurPipelineLayout;
    };

    Pipelines createAllPipelines(
        VulkanWindow const& window,
        PipelineLayouts const& pipelineLayouts
    );

    PipelineLayout createPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout materialLayout);
    PipelineLayout createPostProcessingPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout postProcessingLayout);
    PipelineLayout createSSAOPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout ssaoLayout);
    PipelineLayout createSSAOBlurPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout ssaoBlurLayout);
    Pipeline createPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    
    Pipeline createDeferredGeometryPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createDeferredGeometryAlphaPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createDeferredLightingPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createFXAAPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createSSRPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createSSAOPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createSSAOHBlurPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createSSAOBlurredPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
}

#endif