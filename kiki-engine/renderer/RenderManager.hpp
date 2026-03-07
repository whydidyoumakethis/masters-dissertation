#ifndef KIKI_RENDERER
#define KIKI_RENDERER

#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Synchronisation.hpp"
#include "WindowInfo.hpp"


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

        std::size_t frameIndex = 0;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<rutils::Fence> frameDone;
        std::vector<rutils::Semaphore> imageAvailable, renderFinished;

        public:
        static RenderManager& get();
        void initialise(WindowInfo info);
        
        void nextFrame();
        void shutdown();
        GLFWwindow* getWindow();

        bool isInitialised() { return initialised; };
    };
}

#endif