#ifndef KIKI_RENDERER
#define KIKI_RENDERER

#include "../Components/MaterialComponent.hpp"
#include "../Components/MeshComponent.hpp"
#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/Allocator.hpp"
#include "utils/Buffer.hpp"
#include "WindowInfo.hpp"

#include "MeshManager.hpp"

#include <glm/glm.hpp>



#define GLM_ENABLE_EXPERIMENTAL


namespace Kiki {
    class RenderManager {
        private:
        RenderManager() = default;
        ~RenderManager() = default;
        RenderManager(const RenderManager&) = delete;
        RenderManager& operator=(const RenderManager&) = delete;

        bool recreateSwapchain = false;
        bool initialised = false;

        rutils::VulkanWindow window;

        rutils::PipelineLayout pipelineLayout;
        rutils::Pipeline pipeline;
        rutils::CommandPool commandPool;

        rutils::Buffer sceneUBO;

        std::size_t frameIndex = 0;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<rutils::Fence> frameDone;
        std::vector<rutils::Semaphore> imageAvailable, renderFinished;
        rutils::DescriptorPool descriptorPool;
        VkDescriptorSet sceneDescriptors;

        rutils::Allocator allocator;

        rutils::Buffer positions, colours;

        public:
        static RenderManager& get();
        void initialise(WindowInfo info = Kiki::WindowInfo{});

        rutils::Buffer createMeshBuffer(std::vector<float> positions);
        rutils::Buffer createMaterialBuffer(std::vector<float> colours);
        
        void draw(MeshComponent meshComponent, MaterialComponent materialComponent, glm::mat4 transformMatrix);
        void nextFrame();
        void shutdown();

        GLFWwindow* getWindow() { return window.window; };
        bool isInitialised() { return initialised; };

        struct SceneUniform {
            glm::mat4 camera;
            glm::mat4 projection;
            glm::mat4 projCam;
        };

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