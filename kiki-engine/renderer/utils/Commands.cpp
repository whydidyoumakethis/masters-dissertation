#include "Commands.hpp"

#include "ToString.hpp"
#include "../../logging/FatalError.hpp"

void imageBarrier(VkCommandBuffer aCmdBuff, VkImage aImage, VkPipelineStageFlags2 aSrcStageMask, VkAccessFlags2 aSrcAccessMask, VkImageLayout aSrcLayout, 
    VkPipelineStageFlags2 aDstStageMask,VkAccessFlags2 aDstAccessMask, VkImageLayout aDstLayout, VkImageSubresourceRange aRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}, 
    std::uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, std::uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED) {

    assert( VK_NULL_HANDLE != aCmdBuff );
    assert( VK_NULL_HANDLE != aImage );
    VkImageMemoryBarrier2 ibarrier{};
    ibarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    ibarrier.srcStageMask = aSrcStageMask;
    ibarrier.srcAccessMask = aSrcAccessMask;
    ibarrier.dstStageMask = aDstStageMask;
    ibarrier.dstAccessMask = aDstAccessMask;
    ibarrier.oldLayout = aSrcLayout;
    ibarrier.newLayout = aDstLayout;
    ibarrier.srcQueueFamilyIndex = aSrcQueueFamilyIndex;
    ibarrier.dstQueueFamilyIndex = aDstQueueFamilyIndex;
    ibarrier.image = aImage;
    ibarrier.subresourceRange = aRange;

    VkDependencyInfo deps{};
    deps.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    deps.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    deps.imageMemoryBarrierCount = 1;
    deps.pImageMemoryBarriers = &ibarrier;

    vkCmdPipelineBarrier2( aCmdBuff, &deps );
}

namespace rutils {
    CommandPool createCommandPool(VulkanWindow const& window, VkCommandPoolCreateFlags flags) {
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex = window.graphicsFamilyIndex;
        poolInfo.flags = flags;

        VkCommandPool cpool = VK_NULL_HANDLE;
        if (auto const res = vkCreateCommandPool(window.device, &poolInfo, nullptr, &cpool); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create command pool\n"
                "vkCreateCommandPool() returned {}", toString(res)
            );
        }

        return CommandPool(window.device, cpool);
    }

    VkCommandBuffer allocCommandBuffer(VulkanWindow const& window, VkCommandPool pool) {
        VkCommandBufferAllocateInfo cbufInfo{};
        cbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cbufInfo.commandPool = pool;
        cbufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cbufInfo.commandBufferCount = 1;

        VkCommandBuffer cbuff = VK_NULL_HANDLE;
        if(auto const res = vkAllocateCommandBuffers(window.device, &cbufInfo, &cbuff); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to allocate command buffer\n"
                "vkAllocateCommandBuffers() returned {}", toString(res)
            );
        }

        return cbuff;
    }

    void recordCommands(VkCommandBuffer aCmdBuff, VkPipeline aGraphicsPipe, ImageAndView const& aColorAttach, VkExtent2D const& aImageExtent) {
        // Begin recording commands
        VkCommandBufferBeginInfo begInfo{};
        begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        begInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer( aCmdBuff, &begInfo ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to begin recording command buffer\n"
                "vkBeginCommandBuffer() returned {}", toString(res)
            );
        }

        // Barrier: Ensure the color attachment image is in the right layout
        imageBarrier( aCmdBuff, aColorAttach.image,
            /* Before */
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        );

        // Begin rendering
        VkRenderingAttachmentInfo colorAttach[1]{};
        colorAttach[0].sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttach[0].imageView = aColorAttach.view;
        colorAttach[0].imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        colorAttach[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttach[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttach[0].clearValue.color.float32[0] = 0.1f;
        colorAttach[0].clearValue.color.float32[1] = 0.1f;
        colorAttach[0].clearValue.color.float32[2] = 0.1f;
        colorAttach[0].clearValue.color.float32[3] = 1.f;
  
        VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.layerCount = 1;
        renderInfo.renderArea.offset = VkOffset2D{ 0, 0 };
        renderInfo.renderArea.extent = aImageExtent;

        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorAttach;

        vkCmdBeginRendering( aCmdBuff, &renderInfo );

        // Begin drawing with our graphics pipeline
        vkCmdBindPipeline( aCmdBuff, VK_PIPELINE_BIND_POINT_GRAPHICS, aGraphicsPipe );

        // Draw a triangle = three vertices
        vkCmdDraw( aCmdBuff, 3, 1, 0, 0 );

        // End rendering
        vkCmdEndRendering( aCmdBuff );

        // Barrier: synchronize with the copy after and transition image is to TRANSFER SRC OPTIMAL
        imageBarrier( aCmdBuff, aColorAttach.image,
            /* Before */
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
        );

        // End command recording
        if (auto const res = vkEndCommandBuffer( aCmdBuff ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to end recording command buffer\n"
                "vkEndCommandBuffer() returned {}", toString(res)
            );
        }
    }

    void submitCommands(VulkanWindow const& window, VkCommandBuffer aCmdBuff, VkFence aFence, VkSemaphore aWaitSemaphore, VkSemaphore aSignalSemaphore) {
        VkSemaphoreSubmitInfo wait[1]{};
        wait[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        wait[0].semaphore = aWaitSemaphore;
        wait[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkSemaphoreSubmitInfo signal[1]{};
        signal[0].sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        signal[0].semaphore = aSignalSemaphore;
        signal[0].stageMask = VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;

        VkCommandBufferSubmitInfo submit[1]{};
        submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit[0].commandBuffer = aCmdBuff;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.waitSemaphoreInfoCount = 1;
        submitInfo.pWaitSemaphoreInfos = wait;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = submit;
        submitInfo.signalSemaphoreInfoCount = 1;
        submitInfo.pSignalSemaphoreInfos = signal;

        if (auto const res = vkQueueSubmit2( window.graphicsQueue, 1, &submitInfo, aFence); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", toString(res)
            );
        }
    }
}