#include "Devices.hpp"

#include "ToString.hpp"
#include "../../logging/Logger.hpp"
#include "../../logging/FatalError.hpp"

#include <vector>
#include <format>


std::vector<char const*> checkRequiredDeviceFeatures(VkPhysicalDevice device) {
    VkPhysicalDeviceVulkan13Features vk13{};
	vk13.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		
	VkPhysicalDeviceVulkan14Features vk14{};
	vk14.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
	vk14.pNext  = &vk13;

	VkPhysicalDeviceFeatures2 feat{};
	feat.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	feat.pNext  = &vk14;

	vkGetPhysicalDeviceFeatures2(device, &feat);

	std::vector<char const*> missingFeat;
	if(!vk13.synchronization2) {
		missingFeat.emplace_back("synchronization2");
	}
	if(!vk13.dynamicRendering) {
		missingFeat.emplace_back("dynamicRendering");
	}
	if(!vk14.maintenance5) {
		missingFeat.emplace_back("maintenance5");
	}

	return missingFeat;
}

float scoreDevice(VkPhysicalDevice device, VkSurfaceKHR surface) {
    VkPhysicalDeviceProperties props;
	vkGetPhysicalDeviceProperties(device, &props);

	// Only consider Vulkan 1.4 devices ...
	auto const major = VK_API_VERSION_MAJOR(props.apiVersion);
	auto const minor = VK_API_VERSION_MINOR(props.apiVersion);

	if( major < 1 || (major == 1 && minor < 4) ) {
        klog::error(std::format("Info: Discarding device '{}': insufficient vulkan version at {}.{}\n", props.deviceName, major, minor));
		return -1.f;
	}

	// ... with the required device features
	if(auto const missing = checkRequiredDeviceFeatures(device); !missing.empty()) {
        std::string msg = std::format("Info: Discarding device '{}': {} required features missing:\n", props.deviceName, missing.size());
        
		for(auto const* feat : missing)
			msg = msg.append(std::format("  - {}\n", feat));

        klog::error(msg);
		return -1.f;
	}

	// Check that the device supports the VK KHR swapchain extension
	auto const exts = rutils::getDeviceExtensions( device );

	if( !exts.count( VK_KHR_SWAPCHAIN_EXTENSION_NAME ) ) {
		std::print( stderr, "Info: Discarding device ’{}’: extension {} missing\n", props.deviceName, VK_KHR_SWAPCHAIN_EXTENSION_NAME );
		return -1.f;
	}

	// Ensure there is a queue family that can present to the given surface
	if( !rutils::findQueueFamily( device, 0, surface ) ) {
		std::print( stderr, "Info: Discarding device ’{}’: can’t present to surface\n", props.deviceName );
		return -1.f;
	}

	// Also ensure there is a queue family that supports graphics commands
	if( !rutils::findQueueFamily( device, VK_QUEUE_GRAPHICS_BIT ) ) {
		std::print( stderr, "Info: Discarding device ’{}’: no graphics queue family\n", props.deviceName );
		return -1.f;
	}

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

	VkDevice createDevice(VkPhysicalDevice aPhysicalDev, std::vector<std::uint32_t> const& aQueues, std::vector<char const*> const& aEnabledExtensions) {
		if( aQueues.empty() )
			throw Kiki::FatalError( "create_device(): no queues requested" );

		float queuePriorities[1] = { 1.f };

		std::vector<VkDeviceQueueCreateInfo> queueInfos( aQueues.size() );
		for( std::size_t i = 0; i < aQueues.size(); ++i ) {
			auto& queueInfo = queueInfos[i];
			queueInfo.sType  = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo.queueFamilyIndex  = aQueues[i];
			queueInfo.queueCount        = 1;
			queueInfo.pQueuePriorities  = queuePriorities;
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		// No extra Vulkan 1.0 features for now.

		VkPhysicalDeviceVulkan13Features vk13{};
		vk13.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
		vk13.synchronization2  = VK_TRUE;
		vk13.dynamicRendering  = VK_TRUE;

		VkPhysicalDeviceVulkan14Features vk14{};
		vk14.sType  = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES;
		vk14.pNext  = &vk13;
		vk14.maintenance5  = VK_TRUE; // Required in Vulkan 1.4, but we need to say that we want it.

		VkDeviceCreateInfo deviceInfo{};
		deviceInfo.sType  = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

		deviceInfo.queueCreateInfoCount     = std::uint32_t(queueInfos.size());
		deviceInfo.pQueueCreateInfos        = queueInfos.data();

		deviceInfo.enabledExtensionCount    = std::uint32_t(aEnabledExtensions.size());
		deviceInfo.ppEnabledExtensionNames  = aEnabledExtensions.data();

		deviceInfo.pEnabledFeatures         = &deviceFeatures;

		deviceInfo.pNext                    = &vk14;

		VkDevice device = VK_NULL_HANDLE;
		if( auto const res = vkCreateDevice( aPhysicalDev, &deviceInfo, nullptr, &device ); VK_SUCCESS != res ) {
			throw Kiki::FatalError( "Unable to create logical device\n"
				"vkCreateDevice() returned {}", toString(res)
			);
		}

		return device;
	}

	std::unordered_set<std::string> getDeviceExtensions( VkPhysicalDevice aPhysicalDev ) {
		std::uint32_t extensionCount = 0;
		if( auto const res = vkEnumerateDeviceExtensionProperties( aPhysicalDev, nullptr, &extensionCount, nullptr ); VK_SUCCESS != res )
		{
			throw Kiki::FatalError( "Unable to get device extension count\n"
				"vkEnumerateDeviceExtensionProperties() returned {}", toString(res)
			);
		}

		std::vector<VkExtensionProperties> extensions( extensionCount );
		if( auto const res = vkEnumerateDeviceExtensionProperties( aPhysicalDev, nullptr, &extensionCount, extensions.data() ); VK_SUCCESS != res )
		{
			throw Kiki::FatalError( "Unable to get device extensions\n"
				"vkEnumerateDeviceExtensionProperties() returned {}", toString(res)
			);
		}

		std::unordered_set<std::string> ret;
		for( auto const& ext : extensions )
			ret.emplace( ext.extensionName );

		return ret;
	}

	std::optional<std::uint32_t> findQueueFamily( VkPhysicalDevice aPhysicalDev, VkQueueFlags aQueueFlags, VkSurfaceKHR aSurface ) {
		std::uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( aPhysicalDev, &numQueues, nullptr );

		std::vector<VkQueueFamilyProperties> families( numQueues );
		vkGetPhysicalDeviceQueueFamilyProperties( aPhysicalDev, &numQueues, families.data());

		for( std::uint32_t i = 0; i < numQueues; ++i ) {
			auto const& family = families[i];

			if( aQueueFlags == (aQueueFlags & family.queueFlags) ) {
				if( VK_NULL_HANDLE == aSurface )
					return i;

				VkBool32 supported = VK_FALSE;
				auto const res = vkGetPhysicalDeviceSurfaceSupportKHR( aPhysicalDev, i, aSurface, &supported );

				if( VK_SUCCESS == res && supported )
					return i;
			}
		}
	}
}
