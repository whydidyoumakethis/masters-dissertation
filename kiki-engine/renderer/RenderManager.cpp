#include "RenderManager.hpp"

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
    RenderManager& RenderManager::get() {
        static RenderManager instance;
        return instance;
    }

    void RenderManager::initialise(WindowInfo info) {
        if (!initialised) {
            initialised = true;

            // Create window
            window = rutils::makeVulkanWindow(info);

            // Initialise resources
            pipelineLayout = rutils::createPipelineLayout(window);
            pipeline = rutils::createPipeline(window, pipelineLayout.handle);
            commandPool = rutils::createCommandPool(window, VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

            for (std::size_t i = 0; i < window.swapImages.size(); i++) {
                commandBuffers.emplace_back(rutils::allocCommandBuffer(window, commandPool.handle));
                frameDone.emplace_back(rutils::createFence(window.device, VK_FENCE_CREATE_SIGNALED_BIT));
                imageAvailable.emplace_back(rutils::createSemaphore(window.device));
                renderFinished.emplace_back(rutils::createSemaphore(window.device));
            }
        }
    }

    void RenderManager::nextFrame() {
        glfwPollEvents();

        if (recreateSwapchain) {
            // We need to destroy several objects, which may still be in use by the GPU. Therefore, first wait for the GPU
            // to finish processing.
            vkDeviceWaitIdle(window.device);

            // Recreate resources
            rutils::recreateSwapchain(window);

            pipeline = rutils::createPipeline(window, pipelineLayout.handle);
            
            recreateSwapchain = false;
        }

        // Advance to next frame
        frameIndex++;
        frameIndex %= commandBuffers.size();

        // Make sure that the frame resources are no longer in use
        assert(frameIndex < frameDone.size());

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

            return;
        }

        if (VK_SUCCESS != acquireRes) {
            throw Kiki::FatalError( "Unable to acquire next swapchain image\n"
                "vkAcquireNextImageKHR() returned {}", rutils::toString(acquireRes)
            );
        }

        // Reset fence
        // Do this only after AcquireNextImage(), so that we can wait on the same fence again in case the swapchain
        // had to be re-created.
        
        if(auto const res = vkResetFences( window.device, 1, &frameDone[frameIndex].handle); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to reset frame fence {}\n"
                "vkResetFences() returned {}", frameIndex, rutils::toString(res)
            );
        }

        // Record and submit commands for this frame
        assert(std::size_t(imageIndex) < window.swapImages.size());

        rutils::ImageAndView colorTarget;
        colorTarget.image = window.swapImages[imageIndex];
        colorTarget.view = window.swapViews[imageIndex];

        assert(std::size_t(frameIndex) < commandBuffers.size());

        rutils::recordCommands(
            commandBuffers[frameIndex],
            pipeline.handle,
            colorTarget,
            window.swapchainExtent
        );

        assert(std::size_t(frameIndex) < renderFinished.size());

        rutils::submitCommands(
            window,
            commandBuffers[frameIndex],
            frameDone[frameIndex].handle,
            imageAvailable[frameIndex].handle,
            renderFinished[frameIndex].handle
        );

        // Present the results
        // https://registry.khronos.org/vulkan/specs/latest/man/html/VkPresentInfoKHR.html
        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinished[frameIndex].handle;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &window.swapchain;
        presentInfo.pImageIndices = &imageIndex;
        presentInfo.pResults = nullptr;

        auto const presentRes = vkQueuePresentKHR(window.presentQueue, &presentInfo);

        if (VK_SUBOPTIMAL_KHR == presentRes || VK_ERROR_OUT_OF_DATE_KHR == presentRes) {
            recreateSwapchain = true;
        }
        else if (VK_SUCCESS != presentRes) {
            throw Kiki::FatalError( "Unable present swapchain image {}\n"
                "vkQueuePresentKHR() returned {}", imageIndex, rutils::toString(presentRes)
            );
        }
    }

    void RenderManager::shutdown() {
        vkDeviceWaitIdle(window.device);

        renderFinished.clear();
        imageAvailable.clear();
        frameDone.clear();

        if (!commandBuffers.empty()) {
            vkFreeCommandBuffers(
                window.device,
                commandPool.handle,
                commandBuffers.size(),
                commandBuffers.data()
            );
            commandBuffers.clear();
        }

        commandPool = {};
        pipeline = {};
        pipelineLayout = {};

        window = {};

        std::cout << "hi :D" << std::endl;
    }

    GLFWwindow* RenderManager::getWindow() {
        return window.window;
    }
}
