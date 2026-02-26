#ifndef KIKI_RENDERER_DEVICES
#define KIKI_RENDERER_DEVICES

#include <volk.h>

#include <vector>
#include <unordered_set>
#include <string>
#include <optional>
#include <cstdint>

namespace rutils {
    VkPhysicalDevice selectDevice(VkInstance instance, VkSurfaceKHR surface);
    VkDevice createDevice(VkPhysicalDevice aPhysicalDev, std::vector<std::uint32_t> const& aQueues, std::vector<char const*> const& aEnabledExtensions);
    std::unordered_set<std::string> getDeviceExtensions(VkPhysicalDevice aPhysicalDev);
    std::optional<std::uint32_t> findQueueFamily( VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface = VK_NULL_HANDLE );
}

#endif