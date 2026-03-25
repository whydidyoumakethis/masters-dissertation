#include "Image.hpp"

#include <bit>
#include <print>
#include <limits>
#include <vector>
#include <utility>
#include <algorithm>

#include <cassert>
#include <cstring> // for std::memcpy()


#include "../../logging/FatalError.hpp"
#include "Synchronisation.hpp"
#include "Commands.hpp"
#include "Buffer.hpp"
#include "ToString.hpp"


namespace rutils {
    Image::Image() noexcept = default;

	Image::~Image() {
        if (VK_NULL_HANDLE != view) {
			// This is a bit of a hack, but means we can just keep the
			// VmaAllocator handle, without also having to store a VkDevice
			// handle (which is indeed already stored in the allocator).
			assert(VK_NULL_HANDLE != mAllocator);

			VmaAllocatorInfo ainfo{};
			vmaGetAllocatorInfo(mAllocator, &ainfo);

			vkDestroyImageView(ainfo.device, view, nullptr);
		}

		if(VK_NULL_HANDLE != image) {
			assert(VK_NULL_HANDLE != mAllocator);
			assert(VK_NULL_HANDLE != allocation);
			vmaDestroyImage(mAllocator, image, allocation);
		}
	}

	Image::Image(VmaAllocator aAllocator, VkImage aImage, VmaAllocation aAllocation, VkImageView aView) noexcept
		: image(aImage)
		, allocation(aAllocation)
		, mAllocator(aAllocator)
        , view(aView)
	{}

	Image::Image( Image&& aOther ) noexcept
		: image(std::exchange(aOther.image, VK_NULL_HANDLE))
		, allocation(std::exchange(aOther.allocation, VK_NULL_HANDLE))
		, mAllocator(std::exchange(aOther.mAllocator, VK_NULL_HANDLE))
        , view(std::exchange(aOther.view, VK_NULL_HANDLE))
	{}

	Image& Image::operator=( Image&& aOther ) noexcept {
		std::swap(image, aOther.image);
		std::swap(allocation, aOther.allocation);
		std::swap(mAllocator, aOther.mAllocator);
        std::swap(view, aOther.view);
		return *this;
	}

    Image loadImageTexture(stbi_uc* imageData, int baseWidthi, int baseHeighti, VulkanWindow const& aContext, VkCommandPool aCmdPool, Allocator const& aAllocator) {
		// // Flip images vertically by default. Vulkan expects the first scanline to be the bottom-most scanline. PNG et al.
        // // instead define the first scanline to be the top-most one.
        // stbi_set_flip_vertically_on_load( 1 );

        // // Load base image
        // int baseWidthi, baseHeighti;// baseChannelsi;

        // stbi_uc* data = stbi_load_from_memory( buffer, bufferLength, &baseWidthi, &baseHeighti, &baseChannelsi, 4 /* want 4 c h a n n e l s = RGBA */);

        // if (!data) {
        //     throw Kiki::FatalError("{}: unable to load texture base image", stbi_failure_reason());
        // }

        auto const baseWidth = std::uint32_t(baseWidthi);
        auto const baseHeight = std::uint32_t(baseHeighti);

        // Create staging buffer and copy image data to it
        auto const sizeInBytes = baseWidth * baseHeight * 4;

        auto staging = createBuffer(aAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        void* sptr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, staging.allocation, &sptr); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Mapping memory for writing\n"
                "vmaMapMemory() returned {}", toString(res)
            );
        }

        std::memcpy(sptr, imageData, sizeInBytes);
        vmaUnmapMemory(aAllocator.allocator, staging.allocation);

        //// Free image data
        //stbi_image_free(imageData);

        // Create image
        Image ret = createImageTexture(aAllocator, baseWidth, baseHeight, VK_FORMAT_R8G8B8A8_SRGB, aContext, 
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT);
        
        // Create command buffer for data upload and begin recording
        VkCommandBuffer cbuff = allocCommandBuffer(aContext, aCmdPool);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(cbuff, &beginInfo); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Beginning command buffer recording\n"
                "vkBeginCommandBuffer() returned {}", toString(res)
            );
        }

        // Transition whole image layout
        // When copying data to the image, the image’s layout must be TRANSFER DST OPTIMAL. The current
        // image layout is UNDEFINED (which is the initial layout the image was created in).
        auto const mipLevels = computeMipLevelCount( baseWidth, baseHeight );

        imageBarrier( cbuff, ret.image,
            /* Before */
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_COPY_BIT | VK_PIPELINE_STAGE_2_BLIT_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            /* Which p a r t s */
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, mipLevels,
                0, 1
            }
        );

        // Upload data from staging buffer to image
        VkBufferImageCopy copy;
        copy.bufferOffset = 0;
        copy.bufferRowLength = 0;
        copy.bufferImageHeight = 0;
        copy.imageSubresource = VkImageSubresourceLayers{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0,
            0, 1
        };
        copy.imageOffset = VkOffset3D{ 0, 0, 0 };
        copy.imageExtent = VkExtent3D{ baseWidth, baseHeight, 1 };

        vkCmdCopyBufferToImage(cbuff, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        // Transition base level to TRANSFER SRC OPTIMAL
        imageBarrier( cbuff, ret.image,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_BLIT_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            /* Which p a r t s */
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 1
            }
        );

        // Process all mipmap levels
        uint32_t width = baseWidth, height = baseHeight;

        for (std::uint32_t level = 1; level < mipLevels; ++level) {
            // Blit previous mipmap level (=level-1) to the current level. Note that the loop starts at level = 1.
            // Level = 0 is the base level that we initialied before the loop.
            VkImageBlit blit{};
                blit.srcSubresource = VkImageSubresourceLayers{
                VK_IMAGE_ASPECT_COLOR_BIT,
                level-1,
                0, 1
            };
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { std::int32_t(width), std::int32_t(height), 1 };

            // Next mip level
            width >>= 1; if( width == 0 ) width = 1;
            height >>= 1; if( height == 0 ) height = 1;

            blit.dstSubresource = VkImageSubresourceLayers{
                VK_IMAGE_ASPECT_COLOR_BIT,
                level,
                0, 1
            };
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { std::int32_t(width), std::int32_t(height), 1 };

            vkCmdBlitImage(cbuff,
                ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            // Transition mip level to TRANSFER SRC OPTIMAL for the next iteration. (Technically this is
            // unnecessary for the last mip level, but transitioning it as well simplifes the final barrier following the
            // loop).
            imageBarrier(cbuff, ret.image,
                /* Before */
                VK_PIPELINE_STAGE_2_BLIT_BIT,
                VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                /* A f t e r */
                VK_PIPELINE_STAGE_2_BLIT_BIT,
                VK_ACCESS_2_TRANSFER_READ_BIT,
                VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                /* Which p a r t s */
                VkImageSubresourceRange{
                    VK_IMAGE_ASPECT_COLOR_BIT,
                    level, 1,
                    0, 1
                }
            );
        }

        // Whole image is currently in the TRANSFER SRC OPTIMAL layout. To use the image as a texture from
        // which we sample, it must be in the SHADER READ ONLY OPTIMAL layout.
        imageBarrier(cbuff, ret.image,
            /* Before */
            VK_PIPELINE_STAGE_2_BLIT_BIT,
            VK_ACCESS_2_TRANSFER_READ_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            /* Which p a r t s */
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, mipLevels,
                0, 1
            }
        );

        // End command recording
        if (auto const res = vkEndCommandBuffer(cbuff); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Ending command buffer recording\n"
                "vkEndCommandBuffer() returned {}", toString(res)
            );
        }

        // Submit command buffer and wait for commands to complete. Commands must have completed before we can
        // destroy the temporary resources, such as the staging buffers.
        Fence uploadComplete = createFence(aContext.device);

        VkCommandBufferSubmitInfo submit[1]{};
        submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit[0].commandBuffer = cbuff;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = submit;

        if (auto const res = vkQueueSubmit2(aContext.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", toString(res)
            );
        }

        if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Waiting for upload to complete\n"
                "vkWaitForFences() returned {}", toString(res)
            );
        }

        // Return resulting image
        // Most temporary resources are destroyed automatically through their destructors. However, the command
        // buffer we must free manually.
        vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cbuff);

        return ret;
	}

	Image createImageTexture(Allocator const& aAllocator, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat aFormat, VulkanWindow const& window, VkImageUsageFlags aUsage) {
		auto const mipLevels = computeMipLevelCount(aWidth, aHeight);

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
            throw Kiki::FatalError( "Unable to allocate image.\n"
                "vmaCreateImage() returned {}", toString(res)
            );
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = aFormat;
        viewInfo.components = VkComponentMapping{}; // == identity
        viewInfo.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, VK_REMAINING_MIP_LEVELS,
            0, 1
        };

        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView( window.device, &viewInfo, nullptr, &view); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create image view\n"
                "vkCreateImageView() returned {}", toString(res)
            );
        }

        return Image(aAllocator.allocator, image, allocation, view);
	}

    Sampler createSampler(VulkanWindow const& aContext) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.minLod = 0.f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;
        samplerInfo.mipLodBias = 0.f;

        VkSampler sampler = VK_NULL_HANDLE;
        if (auto const res = vkCreateSampler(aContext.device, &samplerInfo, nullptr, &sampler); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create sampler\n"
                "vkCreateSampler() returned {}", toString(res)
            );
        }

        return Sampler(aContext.device, sampler);
    }

	std::uint32_t computeMipLevelCount(std::uint32_t aWidth, std::uint32_t aHeight) {
		std::uint32_t const bits = aWidth | aHeight;
		std::uint32_t const leadingZeros = std::countl_zero( bits );
		return 32-leadingZeros;
	}
    
	Image createDepthBuffer(VulkanWindow const& window, Allocator const& allocator) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.extent.width = window.swapchainExtent.width;
        imageInfo.extent.height = window.swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.flags = 0;
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (auto const res = vmaCreateImage(allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to allocate depth buffer image.\n"
                "vmaCreateImage() returned {}", toString(res)
            );
        }

        Image depthBuffer( allocator.allocator, image, allocation );

        // Create the image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = depthBuffer.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.components = VkComponentMapping{};
        viewInfo.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_DEPTH_BIT,
            0, 1,
            0, 1
        };

        if (auto const res = vkCreateImageView(window.device, &viewInfo, nullptr, &depthBuffer.view); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create image view\n"
                "vkCreateImageView() returned {}", toString(res)
            );
        }

        return depthBuffer;
    }
}