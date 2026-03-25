#ifndef KIKI_RENDERER
#define KIKI_RENDERER

#include "../Components/MaterialComponent.hpp"
#include "../Components/MeshComponent.hpp"
#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/Allocator.hpp"
#include "utils/Buffer.hpp"
#include "utils/Image.hpp"
#include "WindowInfo.hpp"
#include "Camera.hpp"
#include "utils/Pipelines.hpp"
#include "GltfLoader/GltfLoaderAssimp.h"


#include <glm/glm.hpp>
#include <stb_image.h>
#include <string.h>
#include <cstring>




#define GLM_ENABLE_EXPERIMENTAL


namespace Kiki {
    struct Mesh {
        rutils::Buffer positions;
        rutils::Buffer texCoords;
        rutils::Buffer normals;
        rutils::Buffer indices;
        std::uint32_t vertexCount;
        std::uint32_t indexCount;
    };

    struct Material {
        rutils::Image texture;
        rutils::Image roughnessMetalness;
        VkDescriptorSet descriptorSet;
    };

    class RenderManager {
        private:
        RenderManager() = default;
        ~RenderManager() = default;
        RenderManager(const RenderManager&) = delete;
        RenderManager& operator=(const RenderManager&) = delete;

        bool recreateSwapchain = false;
        bool initialised = false;

        rutils::VulkanWindow window;

        // rutils::PipelineLayout pipelineLayout;
        // rutils::Pipeline pipeline;
        // rutils::Pipeline alphaPipeline;
        rutils::PipelineLayouts pipelineLayouts;

        rutils::Pipelines pipelines;
        rutils::DescriptorSetLayout gBufferLayout;
        

        rutils::CommandPool commandPool;

        rutils::Buffer sceneUBO;

        std::size_t frameIndex = 0;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<rutils::Fence> frameDone;
        std::vector<rutils::Semaphore> imageAvailable, renderFinished;
        rutils::DescriptorPool descriptorPool;
        rutils::DescriptorSetLayout materialLayout;
        VkDescriptorSet sceneDescriptors;

        rutils::Image depthBuffer;
        rutils::Allocator allocator;
        rutils::Sampler sampler;

        rutils::Image noTexture;
        VkDescriptorSet noTextureDst;

        Camera camera; // default cam

        public:
        static RenderManager& get();
        void initialise(WindowInfo info = Kiki::WindowInfo{});

        Mesh allocateMesh(std::vector<float> positions, std::vector<std::uint32_t> indices, std::vector<float> normals, std::vector<float> texCoords);
        Material allocateMaterial(const Mtexture& materialData);
        
        void draw(MeshComponent meshComponent, MaterialComponent materialComponent, glm::mat4 transformMatrix);
        void nextFrame();
        void shutdown();

        void setCamera(Camera&);

        VkDevice& getDevice() { return window.device; };
        GLFWwindow* getWindow() { return window.window; };
        bool isInitialised() { return initialised; };

        struct SceneUniform {
            glm::mat4 camera;
            glm::mat4 projection;
            glm::mat4 projCam;
            glm::vec4 lightPos;
            glm::vec4 lightColour;
            glm::vec4 cameraPos;
        };

        rutils::CommandPool tempTextureCmdPool;

        // We want to use vkCmdUpdateBuffer() to update the contents of our uniform buffers. vkCmdUpdateBuffer()
        // has a number of requirements, including the two below. See
        // https://registry.khronos.org/vulkan/specs/latest/man/html/vkCmdUpdateBuffer.html
        static_assert(sizeof(SceneUniform) <= 65536, "SceneUniform must be less than 65536 bytes for vkCmdUpdateBuffer");
        static_assert(sizeof(SceneUniform) % 4 == 0, "SceneUniform size must be a multiple of 4 bytes");

        private:
        void updateSceneUniforms(SceneUniform& aSceneUniforms, std::uint32_t aFramebufferWidth, std::uint32_t aFramebufferHeight);
    };
}

#endif