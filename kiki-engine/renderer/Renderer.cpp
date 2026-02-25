#include "Renderer.hpp"

#include "utils/VulkanWindow.hpp"
#include "utils/VulkanWrapper.hpp"
#include "utils/Pipelines.hpp"
#include "utils/Commands.hpp"
#include "utils/Synchronisation.hpp"
#include "utils/ToString.hpp"
#include "../logging/FatalError.hpp"

#include <iostream>


void glfwCallback(GLFWwindow*, int, int, int, int);

namespace Kiki {
    void initialiseRenderer() {
        rutils::VulkanWindow window = rutils::makeVulkanWindow();

        // temporary rendering loop
        // create glfw callback
        glfwSetKeyCallback(window.window, &glfwCallback);

        // Initialise resources
        rutils::PipelineLayout pipelineLayout = rutils::createPipelineLayout(window);
        rutils::Pipeline pipeline = rutils::createPipeline(window, pipelineLayout.handle);
        rutils::CommandPool pool = rutils::createCommandPool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

        std::size_t frameIndex = 0;
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<rutils::Fence> frameDone;
        std::vector<rutils::Semaphore> imageAvailable, renderFinished;

        for (std::size_t i = 0; i < window.swapImages.size(); i++) {
            commandBuffers.emplace_back(rutils::allocCommandBuffer(window, pool.handle));
            frameDone.emplace_back(rutils::createFence(window.device, VK_FENCE_CREATE_SIGNALED_BIT));
            imageAvailable.emplace_back(rutils::createSemaphore(window.device));
            renderFinished.emplace_back(rutils::createSemaphore(window.device));
        }

        // Application loop
        bool recreateSwapchain = false;

        while (!glfwWindowShouldClose(window.window)) {
            glfwPollEvents();

            if (recreateSwapchain) {
                recreateSwapchain = false;
            }

            // Advance to next frame
            frameIndex++;
            frameIndex %= commandBuffers.size();

            // Make sure that the frame resources are no longer in use
            assert( frameIndex < frameDone.size() );

            if (auto const res = vkWaitForFences(window.device, 1, &frameDone[frameIndex].handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
                throw Kiki::FatalError( "Unable to wait for frame fence {}\n"
                    "vkWaitForFences() returned {}", frameIndex, rutils::toString(res)
                );
            }

            // Acquire next swap chain image
            assert(frameIndex < imageAvailable.size());

            std::uint32_t imageIndex = 0;
            auto const acquireRes = vkAcquireNextImageKHR(
                window.device,
                window.swapchain,
                std::numeric_limits<std::uint64_t>::max(),
                imageAvailable[frameIndex].handle,
                VK_NULL_HANDLE,
                &imageIndex
            );

            if (VK_SUBOPTIMAL_KHR == acquireRes || VK_ERROR_OUT_OF_DATE_KHR == acquireRes) {
                // This occurs e.g., when the window has been resized. In this case we need to recreate the swap chain to
                // match the new dimensions. Any resources that directly depend on the swap chain need to be recreated
                // as well. While rare, re-creating the swap chain may give us a different image format, which we should
                // handle appropriately.
                //
                // In both cases, we set the flag that the swap chain has to be re-created and jump to the top of the loop.
                // Technically, with the VK SUBOPTIMAL KHR return code, we could continue rendering with the
                // current swap chain (unlike VK ERROR OUT OF DATE KHR, which does require us to recreate the
                // swap chain).
                recreateSwapchain = true;

                // We won’t render a frame this time around. Consequently, no commands were submitted for execution
                // and the associated fence won’t be signalled. Stepping back one frame avoids this problem.
                --frameIndex;
                frameIndex %= commandBuffers.size();

                continue;
            }

            if (VK_SUCCESS != acquireRes) {
                throw Kiki::FatalError( "Unable to acquire next swapchain image\n"
                    "vkAcquireNextImageKHR() returned {}", rutils::toString(acquireRes)
                );
            }

        }

        vkDeviceWaitIdle(window.device);

        std::cout << "hi :D" << std::endl;
    }
}

// called everytime key is pressed
void glfwCallback(GLFWwindow* aWindow, int aKey, int /*aScanCode*/, int aAction, int /*aModifierFlags*/) {
    if (GLFW_KEY_ESCAPE == aKey && GLFW_PRESS == aAction) {
		glfwSetWindowShouldClose(aWindow, GLFW_TRUE);
	}
}