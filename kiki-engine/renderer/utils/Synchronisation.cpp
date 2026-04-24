#include "Synchronisation.hpp"

#include "../../logging/FatalError.hpp"
#include "ToString.hpp"

namespace rutils {
    Fence createFence(VkDevice device, VkFenceCreateFlags flags) {
        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = flags;

        VkFence fence = VK_NULL_HANDLE;
        if (auto const res = vkCreateFence(device, &fenceInfo, nullptr, &fence); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create fence\n"
             "vkCreateFence() returned {}", toString(res)
            );
        }

        return Fence(device, fence);
    }

    Semaphore createSemaphore(VkDevice device) {
        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkSemaphore semaphore = VK_NULL_HANDLE;
        if (auto const res = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &semaphore); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create semaphore\n"
                "vkCreateSemaphore() returned {}", toString(res)
            );
        }

        return Semaphore(device, semaphore);
    }

    void imageBarrier(VkCommandBuffer aCmdBuff, VkImage aImage, VkPipelineStageFlags2 aSrcStageMask, VkAccessFlags2 aSrcAccessMask, VkImageLayout aSrcLayout, 
        VkPipelineStageFlags2 aDstStageMask,VkAccessFlags2 aDstAccessMask, VkImageLayout aDstLayout, VkImageSubresourceRange aRange, 
        std::uint32_t aSrcQueueFamilyIndex, std::uint32_t aDstQueueFamilyIndex) {

        assert(VK_NULL_HANDLE != aCmdBuff);
        assert(VK_NULL_HANDLE != aImage);

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

        vkCmdPipelineBarrier2(aCmdBuff, &deps);
    }

    void bufferBarrier(VkCommandBuffer aCmdBuff, VkBuffer aBuffer, VkPipelineStageFlags2 aSrcStageMask, VkAccessFlags2 aSrcAccessMask, VkPipelineStageFlags2 aDstStageMask,
        VkAccessFlags2 aDstAccessMask, VkDeviceSize aSize, VkDeviceSize aOffset, uint32_t aSrcQueueFamilyIndex, uint32_t aDstQueueFamilyIndex) {

        VkBufferMemoryBarrier2 bbarrier{};
        bbarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
        bbarrier.srcStageMask = aSrcStageMask;
        bbarrier.srcAccessMask = aSrcAccessMask;
        bbarrier.dstStageMask = aDstStageMask;
        bbarrier.dstAccessMask = aDstAccessMask;
        bbarrier.srcQueueFamilyIndex = aSrcQueueFamilyIndex;
        bbarrier.dstQueueFamilyIndex = aDstQueueFamilyIndex;
        bbarrier.buffer = aBuffer;
        bbarrier.offset = aOffset;
        bbarrier.size = aSize;

        VkDependencyInfo deps{};
        deps.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        deps.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        deps.bufferMemoryBarrierCount = 1;
        deps.pBufferMemoryBarriers = &bbarrier;

        vkCmdPipelineBarrier2(aCmdBuff, &deps);
    }
}