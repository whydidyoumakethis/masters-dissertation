#ifndef KIKI_RENDERER_BUFFER
#define KIKI_RENDERER_BUFFER

#include <volk.h>
#include <vk_mem_alloc.h>

#include "Allocator.hpp"

namespace rutils {
    class Buffer final {
		public:
        Buffer() noexcept, ~Buffer();

        explicit Buffer( VmaAllocator, VkBuffer = VK_NULL_HANDLE, VmaAllocation = VK_NULL_HANDLE ) noexcept;

        Buffer( Buffer const& ) = delete;
        Buffer& operator= (Buffer const&) = delete;

        Buffer( Buffer&& ) noexcept;
        Buffer& operator = (Buffer&&) noexcept;

		public:
        VkBuffer buffer = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

		private:
        VmaAllocator mAllocator = VK_NULL_HANDLE;
	};

    Buffer createBuffer(Allocator const& aAllocator, VkDeviceSize aSize, VkBufferUsageFlags aBufferUsage, VmaAllocationCreateFlags aMemoryFlags, VmaMemoryUsage aMemoryUsage = VMA_MEMORY_USAGE_AUTO);
}

#endif