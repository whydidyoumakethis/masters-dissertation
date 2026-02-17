#include "vkimage.hpp"

#include <bit>
#include <print>
#include <limits>
#include <vector>
#include <utility>
#include <algorithm>

#include <cassert>
#include <cstring> // for std::memcpy()

#include <stb_image.h>

#include "error.hpp"
#include "synch.hpp"
#include "commands.hpp"
#include "vkbuffer.hpp"
#include "to_string.hpp"


namespace labut2
{
	Image::Image() noexcept = default;

	Image::~Image()
	{
		if( VK_NULL_HANDLE != image )
		{
			assert( VK_NULL_HANDLE != mAllocator );
			assert( VK_NULL_HANDLE != allocation );
			vmaDestroyImage( mAllocator, image, allocation );
		}
	}

	Image::Image( VmaAllocator aAllocator, VkImage aImage, VmaAllocation aAllocation ) noexcept
		: image( aImage )
		, allocation( aAllocation )
		, mAllocator( aAllocator )
	{}

	Image::Image( Image&& aOther ) noexcept
		: image( std::exchange( aOther.image, VK_NULL_HANDLE ) )
		, allocation( std::exchange( aOther.allocation, VK_NULL_HANDLE ) )
		, mAllocator( std::exchange( aOther.mAllocator, VK_NULL_HANDLE ) )
	{}
	Image& Image::operator=( Image&& aOther ) noexcept
	{
		std::swap( image, aOther.image );
		std::swap( allocation, aOther.allocation );
		std::swap( mAllocator, aOther.mAllocator );
		return *this;
	}


	ImageWithView::ImageWithView() noexcept = default;

	ImageWithView::~ImageWithView()
	{
		if( VK_NULL_HANDLE != view )
		{
			// This is a bit of a hack, but means we can just keep the
			// VmaAllocator handle, without also having to store a VkDevice
			// handle (which is indeed already stored in the allocator).
			assert( VK_NULL_HANDLE != mAllocator );

			VmaAllocatorInfo ainfo{};
			vmaGetAllocatorInfo( mAllocator, &ainfo );

			vkDestroyImageView( ainfo.device, view, nullptr );
		}
	}

	ImageWithView::ImageWithView( Image&& aImage, VkImageView aView ) noexcept
		: Image( std::move(aImage) )
		, view( aView )
	{}
	ImageWithView::ImageWithView( VmaAllocator aAllocator, VkImage aImage, VmaAllocation aAllocation, VkImageView aView ) noexcept
		: Image( aAllocator, aImage, aAllocation )
		, view( aView )
	{}

	ImageWithView::ImageWithView( ImageWithView&& aOther ) noexcept
		: Image( std::move(aOther) )
		, view( std::exchange( aOther.view, VK_NULL_HANDLE ) )
	{}

	ImageWithView& ImageWithView::operator= (ImageWithView&& aOther) noexcept
	{
		static_cast<Image&>(*this) = std::move(aOther);
		std::swap( view, aOther.view );
		return *this;
	}
}

namespace labut2
{
	Image load_image_texture2d(char const* aPath, VulkanContext const& aContext, VkCommandPool aCmdPool, Allocator const& aAllocator, VkFormat aFormat) {
		// flip images vertically by default
		// Vulkan expects the first scanline to be the bottom-most scanline
		stbi_set_flip_vertically_on_load(1);

		// load base image
		int baseWidthi, baseHeighti, baseChannelsi;
		stbi_uc* data = stbi_load(aPath, &baseWidthi, &baseHeighti, &baseChannelsi, 4); // 4 channels = RGBA

		if (!data) {
			throw Error("{}: unable to load texture base image({})", aPath, stbi_failure_reason());
		}

		auto const baseWidth = std::uint32_t(baseWidthi);
		auto const baseHeight = std::uint32_t(baseHeighti);

		// create staging buffer and copy image data to it
		auto const sizeInBytes = baseWidth * baseHeight * 4;

		auto staging = create_buffer(aAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

		void* sptr = nullptr;
		if (auto const res = vmaMapMemory(aAllocator.allocator, staging.allocation, &sptr); VK_SUCCESS != res) {
			throw Error("Mapping memory for writing\n" "vmaMapMemory() returned {}", to_string(res));
		}

		std::memcpy(sptr, data, sizeInBytes);
		vmaUnmapMemory(aAllocator.allocator, staging.allocation);

		// free image data
		stbi_image_free(data);

		// create image
		Image ret = create_image_texture2d(aAllocator, baseWidth, baseHeight, aFormat, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);

		// create command buffer for data upload and begin recording
		VkCommandBuffer cbuff = alloc_command_buffer(aContext, aCmdPool);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (auto const res = vkBeginCommandBuffer(cbuff, &beginInfo); VK_SUCCESS != res) {
			throw Error("Beginning command buffer recording\n" "vkBeginCommandBuffer() returned {}", to_string(res));
		}

		// transition whole image layout
		// when copying data to the image, the image's layout must be TRANSFER_DST_OPTIMAL
		// the current image layout is UNDEFINED, which is the initial layout the image was created in
		auto const mipLevels = compute_mip_level_count(baseWidth, baseHeight);

		image_barrier(cbuff, ret.image,
			// before
			VK_PIPELINE_STAGE_2_NONE,
			VK_ACCESS_2_NONE,
			VK_IMAGE_LAYOUT_UNDEFINED,
			// after
			VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			// which parts
			VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, mipLevels,
				0, 1
			}
		);

		// upload data from staging buffer to image
		VkBufferImageCopy copy;
		copy.bufferOffset = 0;
		copy.bufferRowLength = 0;
		copy.bufferImageHeight = 0;
		copy.imageSubresource = VkImageSubresourceLayers{
			VK_IMAGE_ASPECT_COLOR_BIT,
			0,
			0, 1
		};
		copy.imageOffset = VkOffset3D{0, 0, 0};
		copy.imageExtent = VkExtent3D{baseWidth, baseHeight, 1};

		vkCmdCopyBufferToImage(cbuff, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

		// transition base level to TRANSFER_SRC_OPTIMAL
		image_barrier(cbuff, ret.image,
			// before
			VK_PIPELINE_STAGE_2_COPY_BIT,
			VK_ACCESS_2_TRANSFER_WRITE_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			// which parts
			VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, 1,
				0, 1
			}
		);

		// process all mipmap levels
		uint32_t width = baseWidth, height = baseHeight;

		for (std::uint32_t level = 1; level < mipLevels; level++) {
			// blit previous mipmap level to the current level
			// level = 0 is the base level that we initialised before the loop
			VkImageBlit blit{};
			blit.srcSubresource = VkImageSubresourceLayers{
				VK_IMAGE_ASPECT_COLOR_BIT,
				level - 1,
				0, 1
			};
			blit.srcOffsets[0] = {0, 0, 0};
			blit.srcOffsets[1] = {std::int32_t(width), std::int32_t(height), 1};

			// next mip level
			width >>= 1;
			if (width == 0) width = 1;
			height >>= 1;
			if (height == 0) height = 1;

			blit.dstSubresource = VkImageSubresourceLayers{
				VK_IMAGE_ASPECT_COLOR_BIT,
				level,
				0, 1
			};
			blit.dstOffsets[0] = {0, 0, 0};
			blit.dstOffsets[1] = {std::int32_t(width), std::int32_t(height), 1};

			vkCmdBlitImage(cbuff,
				ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR
			);

			// transition mip level to TRANSFER_SRC_OPTIMAL for the next iteration
			image_barrier(cbuff, ret.image,
				// before
				VK_PIPELINE_STAGE_2_BLIT_BIT,
				VK_ACCESS_2_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				// after
				VK_PIPELINE_STAGE_2_BLIT_BIT,
				VK_ACCESS_2_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				// which parts
				VkImageSubresourceRange{
					VK_IMAGE_ASPECT_COLOR_BIT,
					level, 1,
					0, 1
				}
			);
		}

		// whole image is currently in the VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL layout
		// to use the image as a texture from which we sample, it must be in the SHADER_READ_ONLY_OPTIMAL layout
		image_barrier(cbuff, ret.image,
			// before
			VK_PIPELINE_STAGE_2_BLIT_BIT,
			VK_ACCESS_2_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			// after
			VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
			VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			// which parts
			VkImageSubresourceRange{
				VK_IMAGE_ASPECT_COLOR_BIT,
				0, mipLevels,
				0, 1
			}
		);

		// end command recording
		if (auto const res = vkEndCommandBuffer(cbuff); VK_SUCCESS != res) {
			throw Error("Ending command buffer recording" "vkEndCommandBuffer() returned {}", to_string(res));
		}

		// submit command buffer and wait for commands to complete
		// commands must have completed before we can destroy the temporary resources, such as the staging buffers
		Fence uploadComplete = create_fence(aContext.device);

		VkCommandBufferSubmitInfo submit[1]{};
		submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
		submit[0].commandBuffer = cbuff;

		VkSubmitInfo2 submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
		submitInfo.commandBufferInfoCount = 1;
		submitInfo.pCommandBufferInfos = submit;

		if (auto const res = vkQueueSubmit2(aContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res) {
			throw Error("Unable to submit command buffer to queue\n" "vkQueueSubmit2() returned {}", to_string(res));
		}

		if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
			throw Error("Waiting for upload to complete\n" "vkWaitForFences() returned {}", to_string(res));
		}

		// return resulting image
		// we must free the command buffer manually, the other temporary resources are destroyed automatically through their destructors
		vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cbuff);

		return ret;
	}

	Image create_image_texture2d(Allocator const& aAllocator, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat aFormat, VkImageUsageFlags aUsage) {
		auto const mipLevels = compute_mip_level_count(aWidth, aHeight);

		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.format = aFormat;
		imageInfo.extent.width = aWidth;
		imageInfo.extent.height = aHeight;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = mipLevels;
		imageInfo.arrayLayers = 1;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.usage = aUsage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VmaAllocationCreateInfo allocInfo{};
		allocInfo.flags = 0;
		allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

		VkImage image = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;

		if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res) {
			throw Error("Unable to allocate image.\n" "vmaCreateImage() returned {}", to_string(res));
		}

		return Image(aAllocator.allocator, image, allocation);
	}

	std::uint32_t compute_mip_level_count( std::uint32_t aWidth, std::uint32_t aHeight )
	{
		std::uint32_t const bits = aWidth | aHeight;
		std::uint32_t const leadingZeros = std::countl_zero( bits );
		return 32-leadingZeros;
	}
}
