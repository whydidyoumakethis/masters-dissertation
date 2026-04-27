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
		# ifdef TRACY_VK_ENABLE
        TracyVkCtx tracyVkCtx,
		# endif
		Pipelines const&,
		PipelineLayouts const&,
		ImageAndView const& swapchainImage,
		Image const& aDepthAttach,
		GBuffers& gbuffers,
		VkExtent2D const&,
		VkBuffer aSceneUBO,
		Kiki::RenderManager::SceneUniform const&,
		VkDescriptorSet aSceneDescriptors,
		VkDescriptorSet ssaoDescriptors,
		VkDescriptorSet ssaoHBlurDescriptors,
		VkDescriptorSet ssaoBlurredDescriptors,
		VkDescriptorSet deferredLightingDescriptors,
		VkDescriptorSet ffxaDescriptors,
		VkDescriptorSet ssrDescriptors,
		VkDescriptorSet tonemapDescriptors,
		VkDescriptorSet noTexture,
		Kiki::Skybox const& skybox,
		Image const& doneLightingImage,
		Image const& doneSSRImage,
		Image const& doneTonemapImage
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