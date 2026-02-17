#include "commands.hpp"

#include "error.hpp"
#include "to_string.hpp"

namespace labut2
{
	CommandPool create_command_pool(VulkanContext const& aContext, VkCommandPoolCreateFlags aFlags) {
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = aContext.graphicsFamilyIndex;
		poolInfo.flags = aFlags;
		
		VkCommandPool cpool = VK_NULL_HANDLE;
		if (auto const res = vkCreateCommandPool(aContext.device, &poolInfo, nullptr, &cpool); VK_SUCCESS != res) {
			throw Error("Unable to create command pool\n" "vkCreateCommandPool() returned {}", to_string(res));
		}

		return CommandPool(aContext.device, cpool);
	}

	VkCommandBuffer alloc_command_buffer(VulkanContext const& aContext, VkCommandPool aCmdPool) {
		VkCommandBufferAllocateInfo cbufInfo{};
		cbufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		cbufInfo.commandPool = aCmdPool;
		cbufInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		cbufInfo.commandBufferCount = 1;

		VkCommandBuffer cbuff = VK_NULL_HANDLE;
		if (auto const res = vkAllocateCommandBuffers(aContext.device, &cbufInfo, &cbuff); VK_SUCCESS != res) {
			throw Error("Unable to allocate command buffer\n" "vkAllocateCommandBuffers() returned {}", to_string(res));
		}

		return cbuff;	}
}
