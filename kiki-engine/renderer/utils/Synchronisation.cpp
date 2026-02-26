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
}