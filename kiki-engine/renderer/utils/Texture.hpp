#ifndef KIKI_RENDERER_TEXTURE
#define KIKI_RENDERER_TEXTURE

#include <volk.h>
#include <vk_mem_alloc.h>
#include <stb_image.h>

#include "VulkanWrapper.hpp"
#include "Allocator.hpp"

namespace rutils {
    class Texture {
		public:
			Texture() noexcept, ~Texture();

			explicit Texture( VmaAllocator, VkImage = VK_NULL_HANDLE, VmaAllocation = VK_NULL_HANDLE, VkImageView = VK_NULL_HANDLE ) noexcept;

			Texture( Texture const& ) = delete;
			Texture& operator= (Texture) = delete;

			Texture( Texture&& ) noexcept;
			Texture& operator = (Texture&&) noexcept;

		public:
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;

		protected:
			VmaAllocator mAllocator = VK_NULL_HANDLE;
	};

	Texture loadImageTexture(stbi_uc* imageData, int baseWidthi, int baseHeighti, VulkanWindow const&, VkCommandPool, Allocator const&);

	Texture loadImageTextureFromFile(std::filesystem::path path, VulkanWindow const&, VkCommandPool, Allocator const&);

	Texture createImageTexture(Allocator const&, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat, VulkanWindow const& window, VkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    Sampler createSampler(VulkanWindow const&);

	std::uint32_t computeMipLevelCount(std::uint32_t aWidth, std::uint32_t aHeight);
}

#endif