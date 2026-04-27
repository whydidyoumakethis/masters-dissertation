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

    Image loadImageTexture(stbi_uc* imageData, int baseWidthi, int baseHeighti, VulkanWindow const& aContext, VkCommandPool aCmdPool, Allocator const& aAllocator, VkFormat format) {
        auto const baseWidth = std::uint32_t(baseWidthi);
        auto const baseHeight = std::uint32_t(baseHeighti);

        // Create staging buffer and copy image data to it
        auto const sizeInBytes = baseWidth * baseHeight * 4;

        auto staging = createBuffer(aAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        void* sptr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, staging.allocation, &sptr); VK_SUCCESS != res) {
            throw Kiki::FatalError("Mapping memory for writing\n"
                "vmaMapMemory() returned {}", toString(res)
            );
        }

        std::memcpy(sptr, imageData, sizeInBytes);
        vmaUnmapMemory(aAllocator.allocator, staging.allocation);

        //// Free image data
        // stbi_image_free(imageData);

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
            throw Kiki::FatalError("Beginning command buffer recording\n"
                "vkBeginCommandBuffer() returned {}", toString(res)
            );
        }

        // Transition whole image layout
        // When copying data to the image, the image’s layout must be TRANSFER DST OPTIMAL. The current
        // image layout is UNDEFINED (which is the initial layout the image was created in).
        auto const mipLevels = computeMipLevelCount(baseWidth, baseHeight);

        imageBarrier(cbuff, ret.image,
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
        imageBarrier(cbuff, ret.image,
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
            level - 1,
            0, 1
            };
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { std::int32_t(width), std::int32_t(height), 1 };

            // Next mip level
            width >>= 1; if (width == 0) width = 1;
            height >>= 1; if (height == 0) height = 1;

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
            throw Kiki::FatalError("Ending command buffer recording\n"
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
            throw Kiki::FatalError("Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", toString(res)
            );
        }

        if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw Kiki::FatalError("Waiting for upload to complete\n"
                "vkWaitForFences() returned {}", toString(res)
            );
        }

        // Return resulting image
        // Most temporary resources are destroyed automatically through their destructors. However, the command
        // buffer we must free manually.
        vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cbuff);

        return ret;
    }

    Image loadFontAtlas(std::vector<uint8_t> data, int atlasSize, VulkanWindow const& aContext, VkCommandPool aCmdPool, Allocator const& aAllocator) {
        // Create staging buffer and copy image data to it
        auto const sizeInBytes = atlasSize * atlasSize * sizeof(uint8_t) * 4;

        auto staging = createBuffer(aAllocator, sizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

        void* sptr = nullptr;
        if (auto const res = vmaMapMemory(aAllocator.allocator, staging.allocation, &sptr); VK_SUCCESS != res) {
            throw Kiki::FatalError("Mapping memory for writing\n"
                "vmaMapMemory() returned {}", toString(res)
            );
        }

        std::memcpy(sptr, data.data(), sizeInBytes);
        vmaUnmapMemory(aAllocator.allocator, staging.allocation);

        // Create image
        Image ret = createFontAtlasTexture(aAllocator, atlasSize, atlasSize, VK_FORMAT_R8G8B8A8_UNORM, aContext,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT); //TODO

        // Create command buffer for data upload and begin recording
        VkCommandBuffer cbuff = allocCommandBuffer(aContext, aCmdPool);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(cbuff, &beginInfo); VK_SUCCESS != res) {
            throw Kiki::FatalError("Beginning command buffer recording\n"
                "vkBeginCommandBuffer() returned {}", toString(res)
            );
        }

        // Transition whole image layout
        // When copying data to the image, the image’s layout must be TRANSFER DST OPTIMAL. The current
        // image layout is UNDEFINED (which is the initial layout the image was created in)

        imageBarrier(cbuff, ret.image,
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
                0, 1,
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
        copy.imageExtent = VkExtent3D{ (unsigned int)atlasSize, (unsigned int)atlasSize, 1 };

        vkCmdCopyBufferToImage(cbuff, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

        imageBarrier(cbuff, ret.image,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            /* A f t e r */
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            /* Which p a r t s */
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 1
            }
        );

        // End command recording
        if (auto const res = vkEndCommandBuffer(cbuff); VK_SUCCESS != res) {
            throw Kiki::FatalError("Ending command buffer recording\n"
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
            throw Kiki::FatalError("Unable to submit command buffer to queue\n"
                "vkQueueSubmit2() returned {}", toString(res)
            );
        }

        if (auto const res = vkWaitForFences(aContext.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw Kiki::FatalError("Waiting for upload to complete\n"
                "vkWaitForFences() returned {}", toString(res)
            );
        }

        // Return resulting image
        // Most temporary resources are destroyed automatically through their destructors. However, the command
        // buffer we must free manually.
        vkFreeCommandBuffers(aContext.device, aCmdPool, 1, &cbuff);

        return ret;
    }

    Image createFontAtlasTexture(Allocator const& aAllocator, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat aFormat, VulkanWindow const& window, VkImageUsageFlags aUsage) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = aFormat;
        imageInfo.extent.width = aWidth;
        imageInfo.extent.height = aHeight;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;

        // 2. Ensure VK_IMAGE_USAGE_TRANSFER_DST_BIT is present
        // This allows vkCmdCopyBufferToImage to work for runtime updates.
        imageInfo.usage = aUsage | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (auto const res = vmaCreateImage(aAllocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res) {
            throw std::runtime_error("Unable to allocate font atlas image");
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = aFormat;

        // Identity mapping is best for RGB MSDF
        viewInfo.components = {
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };

        viewInfo.subresourceRange = {
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1, // mipLevels
            0, 1  // layers
        };

        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView(window.device, &viewInfo, nullptr, &view); VK_SUCCESS != res) {
            throw std::runtime_error("Unable to create font atlas view");
        }

        return Image(aAllocator.allocator, image, allocation, view);
    }

    Image loadCubemapTexture(std::array<stbi_uc*, 6> faces, uint32_t width, uint32_t height, VulkanWindow const& context, VkCommandPool commandPool, Allocator const& allocator) {        
        // Create staging buffer and copy image data to it
        uint32_t const faceSizeInBytes = width * height * 4;
        uint32_t const totalSizeInBytes = width * height * 24; // total size is 4 channels * 6 images * width per image * height per image

        auto staging = createBuffer(allocator, totalSizeInBytes, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        void* sptr = nullptr;
        if (auto const res = vmaMapMemory(allocator.allocator, staging.allocation, &sptr); res != VK_SUCCESS) {
            throw Kiki::FatalError("Mapping memory for writing\n" "vmaMapMemory() returned {}", toString(res));
        }

        for (int i = 0; i < 6; i++) {
            std::memcpy((char *)sptr + (faceSizeInBytes * i), faces[i], faceSizeInBytes);
        }

        vmaUnmapMemory(allocator.allocator, staging.allocation);

        // for (int i = 0; i < 6; i++) {
        //     stbi_image_free(faces[i]);
        // }

        // Create image        
        Image ret = createImageTexture(
            allocator,
            width,
            height,
            VK_FORMAT_R8G8B8A8_SRGB,
            context, 
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
            6,
            VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT,
            VK_IMAGE_VIEW_TYPE_CUBE
        );
        
        // Create command buffer for data upload and begin recording
        VkCommandBuffer cbuff = allocCommandBuffer(context, commandPool);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        if (auto const res = vkBeginCommandBuffer(cbuff, &beginInfo); res != VK_SUCCESS) {
            throw Kiki::FatalError( "Beginning command buffer recording\n"
                "vkBeginCommandBuffer() returned {}", toString(res)
            );
        }

        // Transition the 6 layers

        imageBarrier( cbuff, ret.image,
            /* Before */
            VK_PIPELINE_STAGE_2_NONE,
            VK_ACCESS_2_NONE,
            VK_IMAGE_LAYOUT_UNDEFINED,
            /* After */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            /* Which parts */
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 6
            }
        );

        std::vector<VkBufferImageCopy> regions;
        
        for (uint32_t i = 0; i < 6; i++) {
            VkBufferImageCopy region{};
            region.bufferOffset = faceSizeInBytes * i;
            region.imageSubresource = VkImageSubresourceLayers{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0,
                i, 1
            };
            region.imageOffset = VkOffset3D{0, 0, 0};
            region.imageExtent = VkExtent3D{width, height, 1};

            regions.push_back(region);
        }

        vkCmdCopyBufferToImage(cbuff, staging.buffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions.data());

        // Transition to SHADER_READ_ONLY_OPTIMAL
        imageBarrier( cbuff, ret.image,
            /* Before */
            VK_PIPELINE_STAGE_2_COPY_BIT,
            VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            /* After */
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
            VK_ACCESS_2_SHADER_SAMPLED_READ_BIT,
            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            /* Which parts */
            VkImageSubresourceRange{
                VK_IMAGE_ASPECT_COLOR_BIT,
                0, 1,
                0, 6
            }
        );

        // End command recording
        if (auto const res = vkEndCommandBuffer(cbuff); VK_SUCCESS != res) {
            throw Kiki::FatalError("Ending command buffer recording\n" "vkEndCommandBuffer() returned {}", toString(res));
        }

        // Submit command buffer and wait for commands to complete. Commands must have completed before we can
        // destroy the temporary resources, such as the staging buffers.
        Fence uploadComplete = createFence(context.device);

        VkCommandBufferSubmitInfo submit[1]{};
        submit[0].sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        submit[0].commandBuffer = cbuff;

        VkSubmitInfo2 submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        submitInfo.commandBufferInfoCount = 1;
        submitInfo.pCommandBufferInfos = submit;

        if (auto const res = vkQueueSubmit2(context.graphicsQueue, 1, &submitInfo, uploadComplete.handle); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to submit command buffer to queue\n" "vkQueueSubmit2() returned {}", toString(res));
        }

        if (auto const res = vkWaitForFences(context.device, 1, &uploadComplete.handle, VK_TRUE, std::numeric_limits<std::uint64_t>::max()); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Waiting for upload to complete\n"
                "vkWaitForFences() returned {}", toString(res)
            );
        }

        // Return resulting image
        // Most temporary resources are destroyed automatically through their destructors. However, the command
        // buffer we must free manually.
        vkFreeCommandBuffers(context.device, commandPool, 1, &cbuff);

        return ret;
	}

	Image createImageTexture(Allocator const& aAllocator, std::uint32_t aWidth, std::uint32_t aHeight, VkFormat aFormat, VulkanWindow const& window, VkImageUsageFlags aUsage, uint32_t layers, VkImageCreateFlags flags, VkImageViewType viewType) {
		auto const mipLevels = computeMipLevelCount(aWidth, aHeight);

        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = aFormat;
        imageInfo.extent.width = aWidth;
        imageInfo.extent.height = aHeight;
        imageInfo.extent.depth = 1;

        if (layers == 1) {
            imageInfo.mipLevels = mipLevels;
        }
        else {
            imageInfo.mipLevels = 1;
        }

        imageInfo.arrayLayers = layers;
        imageInfo.flags = flags;
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
        viewInfo.viewType = viewType;
        viewInfo.format = aFormat;
        viewInfo.components = VkComponentMapping{}; // == identity

        viewInfo.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, imageInfo.mipLevels,
            0, layers
        };

        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView( window.device, &viewInfo, nullptr, &view); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create image view\n"
                "vkCreateImageView() returned {}", toString(res)
            );
        }

        return Image(aAllocator.allocator, image, allocation, view);
	}

    Sampler createSampler(VulkanWindow const& aContext, bool isCubemapSampler) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;

        if (isCubemapSampler) {
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;   
        }

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

    Sampler createFontSampler(VulkanWindow const& aContext) {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;

        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST; 
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        VkSampler sampler = VK_NULL_HANDLE;
        if (auto const res = vkCreateSampler(aContext.device, &samplerInfo, nullptr, &sampler); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create sampler\n"
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
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

    Image createPostProcessingImage(VulkanWindow const& window, Allocator const& allocator) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = window.swapchainFormat;
        imageInfo.extent.width = window.swapchainExtent.width;
        imageInfo.extent.height = window.swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
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

        Image postProcessingImage( allocator.allocator, image, allocation );

        // Create the image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = postProcessingImage.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = window.swapchainFormat;
        viewInfo.components = VkComponentMapping{};
        viewInfo.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1,
            0, 1
        };

        if (auto const res = vkCreateImageView(window.device, &viewInfo, nullptr, &postProcessingImage.view); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create image view\n"
                "vkCreateImageView() returned {}", toString(res)
            );
        }

        return postProcessingImage;
    }

    GBuffers createAllGBufferImages(VulkanWindow const& window, Allocator const& allocator) {
        GBuffers gbuffers;

        gbuffers.textureColour = createGBufferImage(window, allocator, VK_FORMAT_R8G8B8A8_UNORM);
        gbuffers.normals = createGBufferImage(window, allocator, VK_FORMAT_R16G16B16A16_SFLOAT);
        gbuffers.roughnessMetalness = createGBufferImage(window, allocator, VK_FORMAT_R8G8_UNORM);
        gbuffers.mappedNormals = createGBufferImage(window, allocator, VK_FORMAT_R16G16B16A16_SFLOAT);
        gbuffers.ssao = createGBufferImage(window, allocator, VK_FORMAT_R16_SFLOAT);
        gbuffers.ssao_hblur = createGBufferImage(window, allocator, VK_FORMAT_R16_SFLOAT);
        gbuffers.ssao_blurred = createGBufferImage(window, allocator, VK_FORMAT_R16_SFLOAT);

        return gbuffers;
    }


    Image createGBufferImage(VulkanWindow const& window, Allocator const& allocator, VkFormat format) {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = format;
        imageInfo.extent.width = window.swapchainExtent.width;
        imageInfo.extent.height = window.swapchainExtent.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.flags = 0;
        allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        
        VkImage image = VK_NULL_HANDLE;
        VmaAllocation allocation = VK_NULL_HANDLE;

        if (auto const res = vmaCreateImage(allocator.allocator, &imageInfo, &allocInfo, &image, &allocation, nullptr); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to allocate render to texture image.\n" "vmaCreateImage() returned {}", toString(res));
        }

        Image gbufferImage(allocator.allocator, image, allocation);

        // create the image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = gbufferImage.image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components = VkComponentMapping{};
        viewInfo.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1,
            0, 1
        };

        if (auto const res = vkCreateImageView(window.device, &viewInfo, nullptr, &gbufferImage.view); VK_SUCCESS != res) {
            throw Kiki::FatalError("Unable to create image view\n" "vkCreateImageView() returned {}", toString(res));			
        }

        return gbufferImage;
    }
}