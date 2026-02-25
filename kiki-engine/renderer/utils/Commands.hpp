#ifndef KIKI_RENDERER_COMMANDS
#define KIKI_RENDERER_COMMANDS

#include "VulkanWrapper.hpp"
#include "VulkanWindow.hpp"

namespace rutils {
    CommandPool createCommandPool(VulkanWindow const& window, VkCommandPoolCreateFlags flags = 0);
    VkCommandBuffer allocCommandBuffer(VulkanWindow const& window, VkCommandPool pool);
}

#endif