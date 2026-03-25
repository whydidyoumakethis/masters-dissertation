#ifndef KIKI_RENDERER_IMAGE
#define KIKI_RENDERER_IMAGE

#include <volk.h>
#include <vk_mem_alloc.h>
#include <stb_image.h>

#include "VulkanWrapper.hpp"
#include "Allocator.hpp"

namespace rutils {
    class Image {
		public:
			Image() noexcept, ~Image();

			explicit Image( VmaAllocator, VkImage = VK_NULL_HANDLE, VmaAllocation = VK_NULL_HANDLE, VkImageView = VK_NULL_HANDLE ) noexcept;

			Image( Image const& ) = delete;
			Image& operator= (Image const&) = delete;

			Image( Image&& ) noexcept;
			Image& operator = (Image&&) noexcept;

		public:
			VkImage image = VK_NULL_HANDLE;
			VkImageView view = VK_NULL_HANDLE;
			VmaAllocation allocation = VK_NULL_HANDLE;

		protected:
			VmaAllocator mAllocator = VK_NULL_HANDLE;
	};

	struct GBuffers {
		rutils::Image textureColour;
		rutils::Image normals;
		rutils::Image roughnessMetalness;
		rutils::Image worldPos;
	};

	Image loadImageTexture(stbi_uc* imageData, int baseWidthi, int baseHeighti, VulkanWindow const&, VkCommandPool, Allocator const&);

	Image createImageTexture(Allocator const&, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat, VulkanWindow const& window, VkImageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);

    Sampler createSampler(VulkanWindow const&);

	std::uint32_t computeMipLevelCount(std::uint32_t aWidth, std::uint32_t aHeight);

	Image createDepthBuffer(VulkanWindow const& window, Allocator const& allocator);
	GBuffers createAllGBufferImages(VulkanWindow const& window, Allocator const& allocator);
	Image createGBufferImage(VulkanWindow const& window, Allocator const& allocator, VkFormat format);
}

#endif