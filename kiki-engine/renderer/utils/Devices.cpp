#include "Devices.hpp"

#include "ToString.hpp"
#include "../../logging/Logger.hpp"
#include "../../logging/FatalError.hpp"

#include <vector>
#include <format>


std::vector<char const*> checkRequiredDeviceFeatures(VkPhysicalDevice device) {
    return std::vector<char const*>();
}

float scoreDevice(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	// Only consider Vulkan 1.4 devices ...
	auto const major = VK_API_VERSION_MAJOR(props.apiVersion);
	auto const minor = VK_API_VERSION_MINOR(props.apiVersion);

	if( major < 1 || (major == 1 && minor < 4) ) {
        log::error(std::format("Info: Discarding device '{}': insufficient vulkan version at {}.{}\n", props.deviceName, major, minor));
		return -1.f;
	}

	// ... with the required device features
	if(auto const missing = checkRequiredDeviceFeatures(device); !missing.empty()) {
        std::string msg = std::format("Info: Discarding device '{}': {} required features missing:\n", props.deviceName, missing.size());
        
		for(auto const* feat : missing)
			msg = msg.append(std::format("  - {}\n", feat));

        log::error(msg);
		return -1.f;
	}

	//TODO: additional checks
	//TODO:  - check that the VK_KHR_swapchain extension is supported
	//TODO:  - check that there is a queue family that can present to the
	//TODO:    given surface
	//TODO:  - check that there is a queue family that supports graphics
	//TODO:    commands

	// Discrete GPU > Integrated GPU > others
	float score = 0.f;

	if(VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU == props.deviceType)
		score += 500.f;
	else if(VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU == props.deviceType)
		score += 100.f;

	return score;
}


namespace rutils {
    VkPhysicalDevice selectDevice(VkInstance instance, VkSurfaceKHR surface) {
        std::uint32_t numDevices = 0;
		if(auto const res = vkEnumeratePhysicalDevices(instance, &numDevices, nullptr); VK_SUCCESS != res) {
			throw Kiki::FatalError( "Unable to get physical device count\n"
				"vkEnumeratePhysicalDevices() returned {}", rutils::toString(res)
			);
		}

		std::vector<VkPhysicalDevice> devices(numDevices, VK_NULL_HANDLE);
		if(auto const res = vkEnumeratePhysicalDevices(instance, &numDevices, devices.data()); VK_SUCCESS != res) {
			throw Kiki::FatalError( "Unable to get physical device list\n"
				"vkEnumeratePhysicalDevices() returned {}", rutils::toString(res)
			);
		}

		float bestScore = -1.0f;
		VkPhysicalDevice bestDevice = VK_NULL_HANDLE;

		for(auto const device : devices) {
			auto const score = scoreDevice(device, surface);
			if( score > bestScore ) {
				bestScore = score;
				bestDevice = device;
			}
		}

		return bestDevice;
    }
}
