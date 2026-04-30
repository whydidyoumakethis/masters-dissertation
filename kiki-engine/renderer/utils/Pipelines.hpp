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
        std::uint32_t pcfSamples;
    };

    struct ShadowData {
        glm::mat4 model;
        glm::ivec4 indices;
    };

    struct BloomData {
        glm::vec4 data;
    };

    struct DebugData {
        std::uint32_t mode;
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
        std::uint32_t samples;
        float radius;
        std::uint32_t blurSize;
    };

    struct FXAASettings {
        float strength;
        int isEnabled;
    };

    struct CompositeSettings {
        float bloomStrength;
    };

    struct ShapeData {
        glm::vec4 colour;
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
        rutils::Pipeline tonemap;
        rutils::Pipeline interfaceShape;
        rutils::Pipeline interfaceText;
        rutils::Pipeline shadowMap;
        rutils::Pipeline bloomDownsample;
        rutils::Pipeline bloomUpsample;
        rutils::Pipeline composite;
        rutils::Pipeline debug;
    };

    struct PipelineLayouts {
        PipelineLayout pbrPipelineLayout;
        PipelineLayout deferredPipelineLayout;
        PipelineLayout skyboxPipelineLayout;
        PipelineLayout postprocessPipelineLayout;
        PipelineLayout ssaoPipelineLayout;
        PipelineLayout ssaoBlurPipelineLayout;
        PipelineLayout tonemapPipelineLayout;
        PipelineLayout interfaceShapeLayout;
        PipelineLayout interfaceTextLayout;
        PipelineLayout shadowMapPipelineLayout;
        PipelineLayout bloomPipelineLayout;
        PipelineLayout compositePipelineLayout;
        PipelineLayout debugPipelineLayout;
    };

    Pipelines createAllPipelines(
        VulkanWindow const& window,
        PipelineLayouts const& pipelineLayouts
    );

    PipelineLayout createPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout materialLayout, VkDescriptorSetLayout animationLayout);
    PipelineLayout createPostProcessingPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout postProcessingLayout);
    PipelineLayout createSSAOPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout sceneLayout, VkDescriptorSetLayout ssaoLayout);
    PipelineLayout createSSAOBlurPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout ssaoBlurLayout);
    PipelineLayout createTonemapPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout tonemapLayout);
    PipelineLayout createShadowMapPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout shadowMatrixLayout, VkDescriptorSetLayout boneLayout);
    PipelineLayout createBloomPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout bloomLayout);
    PipelineLayout createCompositePipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout compositeLayout);
    PipelineLayout createDebugPipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout debugLayout);
    void createInterfacePipelineLayout(VulkanWindow const& window, VkDescriptorSetLayout interfaceLayout, VkDescriptorSetLayout textLayout, PipelineLayouts* layouts);
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
    Pipeline createTonemapPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createCompositePipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createShadowMapPipeline(VulkanWindow const& window, VkPipelineLayout pipelineLayout);
    Pipeline createBloomDownsamplePipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout);
    Pipeline createBloomUpsamplePipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout);
    Pipeline createDebugPipeline(VulkanWindow const& aWindow, VkPipelineLayout aPipelineLayout);

    Pipeline createInterfacePipeline(VulkanWindow const& window, VkPipelineLayout layout, std::filesystem::path fShaderPath);
}

#endif