#include "Allocator.hpp"

#include <utility>
#include <cassert>

#include "../../logging/FatalError.hpp"
#include "ToString.hpp"


namespace rutils {
	Allocator::Allocator() noexcept = default;

	Allocator::~Allocator() {
		if( VK_NULL_HANDLE != allocator ) {
			vmaDestroyAllocator( allocator );
		}
	}

	Allocator::Allocator( VmaAllocator aAllocator ) noexcept
		: allocator( aAllocator )
	{}

	Allocator::Allocator( Allocator&& aOther ) noexcept
		: allocator( std::exchange( aOther.allocator, VK_NULL_HANDLE ) )
	{}

	Allocator& Allocator::operator=( Allocator&& aOther ) noexcept {
		std::swap( allocator, aOther.allocator );
		return *this;
	}

	Allocator createAllocator(VulkanWindow const& window) {
		VkPhysicalDeviceProperties props{};
		vkGetPhysicalDeviceProperties(window.physicalDevice, &props);

		VmaVulkanFunctions functions{};
		functions.vkGetInstanceProcAddr   = vkGetInstanceProcAddr;
		functions.vkGetDeviceProcAddr     = vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocInfo{};
		allocInfo.vulkanApiVersion  = props.apiVersion;
		allocInfo.physicalDevice    = window.physicalDevice;
		allocInfo.device            = window.device;
		allocInfo.instance          = window.instance;
		allocInfo.pVulkanFunctions  = &functions;
		
		VmaAllocator allocator = VK_NULL_HANDLE;
		if (auto const res = vmaCreateAllocator(&allocInfo, &allocator); VK_SUCCESS != res) {
			throw Kiki::FatalError( "Unable to create allocator\n"
				"vmaCreateAllocator() returned {}", toString(res)
			);
		}

		return Allocator(allocator);
	}
}

