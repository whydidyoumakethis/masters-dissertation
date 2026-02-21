#ifndef KIKI_RENDERER_VULKANWINDOW
#define KIKI_RENDERER_VULKANWINDOW

#include <volk.h>

#if !defined(GLFW_INCLUDE_NONE)
#	define GLFW_INCLUDE_NONE 1
#endif
#include <GLFW/glfw3.h>

#include <vector>
//#include <cstdint>

namespace rutils {
	class VulkanWindow final {
		public:
			VulkanWindow(), ~VulkanWindow();

			// Move-only
			VulkanWindow(VulkanWindow const&) = delete;
			VulkanWindow& operator= (VulkanWindow const&) = delete;

			VulkanWindow(VulkanWindow&&) noexcept;
			VulkanWindow& operator= (VulkanWindow&&) noexcept;

		public:
            VkInstance instance = VK_NULL_HANDLE;
			VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
			VkDevice device = VK_NULL_HANDLE;

			std::uint32_t graphicsFamilyIndex = 0;
			VkQueue graphicsQueue = VK_NULL_HANDLE;
			
			VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;

			GLFWwindow* window = nullptr;
			VkSurfaceKHR surface = VK_NULL_HANDLE;

			std::uint32_t presentFamilyIndex = 0;
			VkQueue presentQueue = VK_NULL_HANDLE;

			VkSwapchainKHR swapchain = VK_NULL_HANDLE;
			std::vector<VkImage> swapImages;
			std::vector<VkImageView> swapViews;

			VkFormat swapchainFormat;
			VkExtent2D swapchainExtent;
	};

	VulkanWindow makeVulkanWindow();


	struct SwapChanges {
		bool changedSize : 1;
		bool changedFormat: 1;
	};

	SwapChanges recreateSwapchain(VulkanWindow&);
}

#endif