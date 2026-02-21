#ifndef KIKI_RENDERER_TOSTRING
#define KIKI_RENDERER_TOSTRING

#include <volk.h>

#include <string>

namespace rutils {
	std::string toString(VkResult);
	std::string toString(VkPhysicalDeviceType);
	std::string toString(VkDebugUtilsMessageSeverityFlagBitsEXT);

	std::string queueFlags(VkQueueFlags);
	std::string messageTypeFlags(VkDebugUtilsMessageTypeFlagsEXT);
	std::string memoryHeapFlags(VkMemoryHeapFlags);
	std::string memoryPropertyFlags(VkMemoryPropertyFlags);

	std::string driverVersion(std::uint32_t aVendorId, std::uint32_t aDriverVersion);
}

#endif