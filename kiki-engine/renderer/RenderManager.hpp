#ifndef KIKI_RENDERER
#define KIKI_RENDERER

#define N_BLOOM_IMAGES 6

#include "Components/MaterialComponent.hpp"
#include "Components/MeshComponent.hpp"
#include "Components/TransparencyComponent.hpp"
#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/Allocator.hpp"
#include "utils/Buffer.hpp"
#include "utils/Image.hpp"
#include "interface/utils/Font.hpp"
#include "WindowInfo.hpp"
#include "Camera.hpp"
#include "utils/Pipelines.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"
#include "debugging/DebugCamera.hpp"
#include "interface/utils/InterfaceTexture.hpp"


#include <glm/glm.hpp>
#include <stb_image.h>
#include <string.h>
#include <cstring>

#include <imgui_impl_vulkan.h>
#include <tracy/Tracy.hpp>
#include <tracy/TracyVulkan.hpp>


#define GLM_ENABLE_EXPERIMENTAL


namespace Kiki {
    enum RenderMode {
        STANDARD,
        TEXTURE_COLOUR,
        MAPPED_NORMALS,
        GEOMETRIC_NORMALS,
        DEPTH,
        METALNESS,
        ROUGHNESS,
        SSAO,
        BLOOM
    };

    enum RenderPreset {
        FAST,
        FANCY,
        ULTRA
    };

    struct RenderSettings {
        int ssaoSamples = 16;
        float ssaoRadius = 0.5f;
        bool ssaoEnabled = true;

        int ssaoBlurRange = 2;

        int shadowPcfSamples = 20;
        bool shadowsEnabled = true;

        int ssrMaxSteps = 16;
        int ssrBinarySteps = 4;
        float ssrStepSize = 0.5f;
        float ssrThicknessTolerance = 0.2f;
        bool ssrEnabled = true;

        float bloomRadius_x = 0.005f;
        float bloomRadius_y = 0.005f;

        float bloomStrength = 0.04f;
        bool bloomEnabled = true;

        bool chromaticAberrationEnabled = false;
        glm::vec2 chromaticRedShift = glm::vec2(0.f, 0.f);
        glm::vec2 chromaticGreenShift = glm::vec2(0.f, 0.f);
        glm::vec2 chromaticBlueShift = glm::vec2(0.f, 0.f);

        float tonemapMaxWhite = 4.f;
        bool tonemapEnabled = true;

        bool customPostprocessEnabled = false;
        int bayerMatrixMode = 2;
        float bayerExposure = 1.0f;
        int bayerLevels = 4;

        float fxaaStrength = 16.f;
        bool fxaaEnabled = true;

        RenderMode renderMode = STANDARD;
        RenderPreset renderPreset = ULTRA;
    };

    struct Mesh {
        rutils::Buffer positions;
        rutils::Buffer texCoords;
        rutils::Buffer normals;
        rutils::Buffer indices;
        rutils::Buffer tangents;
        rutils::Buffer boneIDs;
        rutils::Buffer weights;
        std::uint32_t vertexCount;
        std::uint32_t indexCount;
    };

    struct Material {
        rutils::Image texture;
        rutils::Image roughnessMetalness;
        rutils::Image normalMap;
        VkDescriptorSet descriptorSet;
        bool hasTexture = false;
        bool hasNormalMap = false;
    };

    struct Skybox {
        rutils::Image cubemap;
        rutils::Sampler sampler;
        VkDescriptorSet descriptorSet;
        Mesh mesh;
        rutils::CubemapPaths paths;
    };

    struct Light {
        glm::vec4 position;
        glm::vec4 colour;
    };

    struct ShadowCubemap {
        rutils::Image cubemap;
        VkImageView arrayView = VK_NULL_HANDLE;
    };

    struct WindowExtent {
        uint32_t width;
        uint32_t height;
    };

    struct ShaderPaths {
        std::filesystem::path pbr_v = "default.vert.spv";
        std::filesystem::path pbr_f = "default.frag.spv";
        std::filesystem::path pbr_alpha_v = "default.vert.spv";
        std::filesystem::path pbr_alpha_f = "alpha.frag.spv";
        std::filesystem::path deferred_geometry_v = "default.vert.spv";
        std::filesystem::path deferred_geometry_f = "deferred_geometry.frag.spv";
        std::filesystem::path deferred_geometry_alpha_v = "default.vert.spv";
        std::filesystem::path deferred_geometry_alpha_f = "deferred_geometry_alpha.frag.spv";
        std::filesystem::path deferred_lighting_v = "fullscreen.vert.spv";
        std::filesystem::path deferred_lighting_f = "deferred_lighting.frag.spv";
        std::filesystem::path fxaa_f = "fxaa.frag.spv";
        std::filesystem::path ssr_f = "ssr.frag.spv";
        std::filesystem::path ssao_f = "ssao.frag.spv";
        std::filesystem::path ssao_hblur_f = "ssao_hblur.frag.spv";
        std::filesystem::path ssao_vblur_f = "ssao_vblur.frag.spv";
        std::filesystem::path tonemap_f = "tonemap.frag.spv";
        std::filesystem::path interface_v = "interface.vert.spv";
        std::filesystem::path interface_shape_f = "interface_shape.frag.spv";
        std::filesystem::path interface_text_f = "interface_text.frag.spv";
        std::filesystem::path interface_texture_f = "interface_texture.frag.spv";
        std::filesystem::path shadowmap_v = "shadowMap.vert.spv";
        std::filesystem::path bloom_downsample_f = "bloom_downsample.frag.spv";
        std::filesystem::path bloom_upsample_f = "bloom_upsample.frag.spv";
        std::filesystem::path composite_f = "composite.frag.spv";
        std::filesystem::path debug_f = "debug.frag.spv";
		std::filesystem::path debug_line_v = "debug_line.vert.spv";
        std::filesystem::path debug_line_f = "debug_line.frag.spv";
        std::filesystem::path custom_postprocess_f = "custom_postprocess.frag.spv";
        std::filesystem::path chromatic_aberration_f = "chromatic_aberration.frag.spv";
    };

    class RenderManager {
        private:
        RenderManager() = default;
        ~RenderManager() = default;
        RenderManager(const RenderManager&) = delete;
        RenderManager& operator=(const RenderManager&) = delete;

        bool recreateSwapchain = false;
        bool initialised = false;

        std::mutex queueMutex;

        rutils::VulkanWindow window;

        rutils::PipelineLayouts pipelineLayouts;

        rutils::Pipelines pipelines;
        rutils::DescriptorSetLayout gBufferLayout;
        rutils::DescriptorSetLayout postProcessingLayout;
        
        rutils::GBuffers gbuffers;

        rutils::CommandPool commandPool;

        rutils::Buffer sceneUBO;
        rutils::Buffer interfaceUBO;
        rutils::Buffer interfaceIndices;
        rutils::Buffer interfaceShapeVertices;

		rutils::Buffer debugLineVertexBuffer;
		VkDeviceSize debugLineVertexBufferSize = 0;
		std::uint32_t debugLineVertexCount = 0;

        std::size_t frameIndex = 0;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<rutils::Fence> frameDone;
        std::vector<rutils::Semaphore> imageAvailable, renderFinished;
        rutils::DescriptorPool descriptorPool;

        rutils::DescriptorSetLayout sceneLayout;
        rutils::DescriptorSetLayout interfaceLayout;
        rutils::DescriptorSetLayout textLayout;
        rutils::DescriptorSetLayout textureLayout;
        rutils::DescriptorSetLayout materialLayout;
        rutils::DescriptorSetLayout cubemapLayout;
        rutils::DescriptorSetLayout ssaoLayout;
        rutils::DescriptorSetLayout ssaoBlurredLayout;
        rutils::DescriptorSetLayout chromaticAberrationLayout;
        rutils::DescriptorSetLayout tonemapLayout;
        rutils::DescriptorSetLayout shadowMatrixLayout;
        rutils::DescriptorSetLayout bloomLayout;
        rutils::DescriptorSetLayout compositeLayout;
        rutils::DescriptorSetLayout debugLayout;
        rutils::DescriptorSetLayout customPostprocessLayout;
        VkDescriptorSet sceneDescriptors;
        VkDescriptorSet interfaceDescriptors;
        VkDescriptorSet deferredLightingDescriptors;
        VkDescriptorSet fxaaDescriptors;
        VkDescriptorSet ssrDescriptors;
        VkDescriptorSet ssaoDescriptors;
        VkDescriptorSet ssaoHBlurDescriptors;
        VkDescriptorSet ssaoBlurredDescriptors;
        VkDescriptorSet chromaticAberrationDescriptors;
        VkDescriptorSet tonemapDescriptors;
        VkDescriptorSet shadowMatrixDescriptors;
        VkDescriptorSet compositeDescriptors;
        VkDescriptorSet debugDescriptors;
        VkDescriptorSet customPostprocessDescriptors;

        rutils::Image doneLightingImage;
        rutils::Image doneSSRImage;
        rutils::Image doneCompositeImage;
        rutils::Image doneChromaticAberrationImage;
        rutils::Image doneTonemapImage;
        rutils::Image doneDebugImage;
        rutils::Image doneCustomPostprocessImage;
        rutils::Image depthBuffer;

        std::array<rutils::Image, N_BLOOM_IMAGES> bloomImages;
        std::array<VkDescriptorSet, N_BLOOM_IMAGES> bloomImageDownsampleDescriptorSets;
        std::array<VkDescriptorSet, N_BLOOM_IMAGES> bloomImageUpsampleDescriptorSets;

        rutils::Buffer shadowMatricesBuffer;

        std::vector<ShadowCubemap> shadowCubemaps;

        rutils::Sampler sampler;
        rutils::Sampler fontSampler;

        rutils::Image noTexture;
        rutils::Image noNormalMap;
        VkDescriptorSet noTextureDst;

        Skybox skybox;

        rutils::DescriptorSetLayout animationLayout;

        rutils::Buffer dummyAnimationBuffer;
        VkDescriptorSet dummyAnimationDesc;
        std::vector<glm::vec4> ssaoSamples;

        # ifdef TRACY_VK_ENABLE
        TracyVkCtx tracyVkCtx;
        # endif

        public:
        std::vector<Light> lights;

        rutils::Allocator allocator;
        static RenderManager& get();

        void setRenderPreset(RenderPreset preset);

        Mesh allocateMesh(
            std::vector<float> positions,
            std::vector<std::uint32_t> indices,
            std::vector<float> normals,
            std::vector<float> texCoords,
            std::vector<float> tangents,
            std::vector<int> boneIDs,    
            std::vector<float> weights    
        );

        rutils::Buffer allocateAnimationBuffer();
        VkDescriptorSet allocateAnimationDescriptorSet(const rutils::Buffer& buffer);

        Mesh allocateSkyboxMesh(std::vector<float> positions, std::vector<std::uint32_t> indices);

        void initialise(WindowInfo info = Kiki::WindowInfo{});
        Material allocateMaterial(const Mtexture& materialData);

        void setCustomSkybox(
            std::filesystem::path right,
            std::filesystem::path left,
            std::filesystem::path up,
            std::filesystem::path down,
            std::filesystem::path front,
            std::filesystem::path back
        );

        void nextFrame();
        void shutdown();

        VkDevice& getDevice() { return window.device; };
        GLFWwindow* getWindow() { return window.window; };
        bool isInitialised() { return initialised; };

        void setDebugInterfaceInit(ImGui_ImplVulkan_InitInfo& info);
        void recreatePipelines();

        iutils::InterfaceTexture loadInterfaceTexture(stbi_uc* imageData, int width, int height);
        void loadFontAtlas(iutils::Font* font, std::vector<uint8_t> atlas);
        rutils::Buffer updateInterfaceVertices(std::vector<float> positions);
        WindowExtent getWindowExtent();

        struct SceneUniform {
            glm::mat4 camera;
            glm::mat4 projection;
            glm::mat4 projCam;
            glm::vec4 lightPos[8];
            glm::vec4 lightColour[8];
            glm::vec4 numLights;
            glm::vec4 cameraPos;
            glm::vec4 ssaoSamples[16];
        };

        struct InterfaceUniform {
            glm::mat4 projection;
        };

        rutils::CommandPool tempTextureCmdPool;

        // We want to use vkCmdUpdateBuffer() to update the contents of our uniform buffers. vkCmdUpdateBuffer()
        // has a number of requirements, including the two below. See
        // https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdUpdateBuffer.html
        static_assert(sizeof(SceneUniform) <= 65536, "SceneUniform must be less than 65536 bytes for vkCmdUpdateBuffer");
        static_assert(sizeof(SceneUniform) % 4 == 0, "SceneUniform size must be a multiple of 4 bytes");

        ShaderPaths shaderPaths;
        SceneUniform sceneUniforms;
        RenderSettings renderSettings;

        private:
        void updateSceneUniforms(SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight);
        void updateShadowMatrices(rutils::Allocator const& allocator, rutils::Buffer const& shadowMatricesBuffer, std::vector<Light>& lights);
        void createSkybox(const rutils::CubemapPaths& paths);
		void updateDebugLineBuffer();

        World& world = World::Get();
        entt::registry& registry = world.Registry();

        DebugCamera& debugCam = DebugCamera::get();
    };
}

#endif