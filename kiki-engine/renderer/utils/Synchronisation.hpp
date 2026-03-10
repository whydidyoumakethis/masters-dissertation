#ifndef KIKI_RENDERER_SYNCHRONISATION
#define KIKI_RENDERER_SYNCHRONISATION

#include "VulkanWrapper.hpp"

namespace rutils {
    Fence createFence(VkDevice device, VkFenceCreateFlags flags = 0);
    Semaphore createSemaphore(VkDevice device);

    void imageBarrier(VkCommandBuffer aCmdBuff, VkImage aImage, VkPipelineStageFlags2 aSrcStageMask, VkAccessFlags2 aSrcAccessMask, VkImageLayout aSrcLayout, 
        VkPipelineStageFlags2 aDstStageMask,VkAccessFlags2 aDstAccessMask, VkImageLayout aDstLayout, VkImageSubresourceRange aRange = VkImageSubresourceRange{VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1}, 
        std::uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, std::uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);

    void bufferBarrier(VkCommandBuffer aCmdBuff, VkBuffer aBuffer, VkPipelineStageFlags2 aSrcStageMask, VkAccessFlags2 aSrcAccessMask, VkPipelineStageFlags2 aDstStageMask,
        VkAccessFlags2 aDstAccessMask, VkDeviceSize aSize = VK_WHOLE_SIZE, VkDeviceSize aOffset = 0, uint32_t aSrcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED, uint32_t aDstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED);
}

#endif