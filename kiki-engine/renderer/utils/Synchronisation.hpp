#ifndef KIKI_RENDERER_SYNCHRONISATION
#define KIKI_RENDERER_SYNCHRONISATION

#include "VulkanWrapper.hpp"

namespace rutils {
    Fence createFence(VkDevice device, VkFenceCreateFlags flags = 0);
    Semaphore createSemaphore(VkDevice device);
}

#endif