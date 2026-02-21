#ifndef KIKI_RENDERER_DEVICES
#define KIKI_RENDERER_DEVICES

#include <volk.h>

namespace rutils {
    VkPhysicalDevice selectDevice(VkInstance instance, VkSurfaceKHR surface);
}

#endif