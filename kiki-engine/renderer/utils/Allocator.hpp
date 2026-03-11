#ifndef KIKI_RENDERER_ALLOCATOR
#define KIKI_RENDERER_ALLOCATOR

#include <volk.h>
#include <vk_mem_alloc.h>

#include "VulkanWindow.hpp"

namespace rutils {
    class Allocator {
		public:
        Allocator() noexcept, ~Allocator();

        explicit Allocator(VmaAllocator) noexcept;

        Allocator(Allocator const&) = delete;
        Allocator& operator=(Allocator const&) = delete;

        Allocator(Allocator&&) noexcept;
        Allocator& operator=(Allocator&&) noexcept;

		public:
		VmaAllocator allocator = VK_NULL_HANDLE;
	};

    Allocator createAllocator(VulkanWindow const& window);
}

#endif