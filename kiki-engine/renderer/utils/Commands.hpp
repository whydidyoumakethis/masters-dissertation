#ifndef KIKI_RENDERER_COMMANDS
#define KIKI_RENDERER_COMMANDS

#include "VulkanWrapper.hpp"
#include "VulkanWindow.hpp"
#include "RenderManager.hpp"

namespace rutils {
    struct ImageAndView {
		VkImage image;
		VkImageView view;
	};

    CommandPool createCommandPool(VulkanWindow const& window, VkCommandPoolCreateFlags flags = 0);
    VkCommandBuffer allocCommandBuffer(VulkanWindow const& window, VkCommandPool pool);

    void recordCommands(
		VkCommandBuffer,
		VkPipeline,
		VkPipeline,
		ImageAndView const&,
		Image const& aDepthAttach,
		VkExtent2D const&,
		VkBuffer aSceneUBO,
		Kiki::RenderManager::SceneUniform const&,
		VkPipelineLayout,
		VkDescriptorSet aSceneDescriptors
	);

	void submitCommands(
		VulkanWindow const&,
		VkCommandBuffer,
		VkFence,
		VkSemaphore,
		VkSemaphore
	);
}

#endif