#ifndef KIKI_RENDERER_DEVICES
#define KIKI_RENDERER_DEVICES

#include <volk.h>

#include <vector>

namespace rutils {
    VkPhysicalDevice selectDevice(VkInstance instance, VkSurfaceKHR surface);
    VkDevice createDevice(VkPhysicalDevice aPhysicalDev, std::vector<std::uint32_t> const& aQueues, std::vector<char const*> const& aEnabledExtensions);
}

#endif