#include "VulkanWindow.hpp"

#include <print>
#include <tuple>
#include <limits>
#include <vector>
#include <utility>
#include <optional>
#include <algorithm>
#include <unordered_set>

#include <iostream>

#include <cassert>

#include "../../logging/FatalError.hpp"
#include "ToString.hpp"
#include "Devices.hpp"


// Define helpers
std::unordered_set<std::string> getInstanceLayers();
std::unordered_set<std::string> getInstanceExtensions();

VkInstance createInstance(std::vector<char const*> const& aEnabledLayers, std::vector<char const*> const& aEnabledExtensions, bool aEnableDebugUtils);
VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance aInstance);

std::tuple<VkSwapchainKHR,VkFormat,VkExtent2D> createSwapchain(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface, VkDevice aDevice, GLFWwindow* aWindow, std::vector<std::uint32_t> const& aQueueFamilyIndices, VkSwapchainKHR aOldSwapchain = VK_NULL_HANDLE);
void getSwapchainImages(VkDevice aDevice, VkSwapchainKHR aSwapchain, std::vector<VkImage>& aImages);
void createSwapchainImageViews(VkDevice aDevice, VkFormat aSwapchainFormat, std::vector<VkImage> const& aImages, std::vector<VkImageView>& aViews);

VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback( VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT aType, VkDebugUtilsMessengerCallbackDataEXT const* aData, void* /*aUserPtr*/ );

std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface);
std::unordered_set<VkPresentModeKHR> getPresentModes(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface);


namespace rutils {
	// VulkanWindow
	VulkanWindow::VulkanWindow() = default;

	VulkanWindow::~VulkanWindow() {
		// Device-related objects
		for (auto const view : swapViews)
			vkDestroyImageView(device, view, nullptr);

		if (VK_NULL_HANDLE != swapchain)
			vkDestroySwapchainKHR(device, swapchain, nullptr);

		if( VK_NULL_HANDLE != device )
			vkDestroyDevice( device, nullptr );

		// Window and related objects
		if (VK_NULL_HANDLE != surface)
			vkDestroySurfaceKHR(instance, surface, nullptr);

		// Instance-related objects
		if( VK_NULL_HANDLE != debugMessenger )
			vkDestroyDebugUtilsMessengerEXT( instance, debugMessenger, nullptr );

		if( VK_NULL_HANDLE != instance )
			vkDestroyInstance( instance, nullptr );

		if (window) {
			glfwDestroyWindow(window);

			// The following assumes that we never create more than one window;
			// if there are multiple windows, destroying one of them would
			// unload the whole GLFW library. Nevertheless, this solution is
			// convenient when only dealing with one window (which we will do
			// in the exercises), as it ensure that GLFW is unloaded after all
			// window-related resources are.
			glfwTerminate();
		}
	}

	VulkanWindow::VulkanWindow( VulkanWindow&& aOther ) noexcept
		: instance( std::exchange( aOther.instance, VK_NULL_HANDLE ) )
		, physicalDevice( std::exchange( aOther.physicalDevice, VK_NULL_HANDLE ) )
		, device( std::exchange( aOther.device, VK_NULL_HANDLE ) )
		, graphicsFamilyIndex( aOther.graphicsFamilyIndex )
		, graphicsQueue( std::exchange( aOther.graphicsQueue, VK_NULL_HANDLE ) )
		, debugMessenger( std::exchange( aOther.debugMessenger, VK_NULL_HANDLE ) )
		, window( std::exchange( aOther.window, VK_NULL_HANDLE ) )
		, surface( std::exchange( aOther.surface, VK_NULL_HANDLE ) )
		, presentFamilyIndex( aOther.presentFamilyIndex )
		, presentQueue( std::exchange( aOther.presentQueue, VK_NULL_HANDLE ) )
		, swapchain( std::exchange( aOther.swapchain, VK_NULL_HANDLE ) )
		, swapImages( std::move( aOther.swapImages ) )
		, swapViews( std::move( aOther.swapViews ) )
		, swapchainFormat( aOther.swapchainFormat )
		, swapchainExtent( aOther.swapchainExtent )
	{}

	VulkanWindow& VulkanWindow::operator=( VulkanWindow&& aOther ) noexcept
	{
		/* We can't just copy over the data from aOther, as we need to ensure that
		 * any potential objects help by `this` are destroyed properly. Swapping
		 * the data of `this` with aOther will do so: the data of `this` ends up in
		 * aOther, and is subsequently destroyed properly by aOther's destructor.
		 *
		 * Advantages are that the move-operation is quite cheap and can trivially
		 * be `noexcept`. Disadvantage is that the destruction of the resources
		 * held by `this` is delayed until aOther's destruction.
		 *
		 * This is a somewhat common way of implementing move assignments.
		 */
		std::swap( instance, aOther.instance );
		std::swap( physicalDevice, aOther.physicalDevice );
		std::swap( device, aOther.device );
		std::swap( graphicsFamilyIndex, aOther.graphicsFamilyIndex );
		std::swap( graphicsQueue, aOther.graphicsQueue );
		std::swap( debugMessenger, aOther.debugMessenger );
		std::swap( window, aOther.window );
		std::swap( surface, aOther.surface );
		std::swap( presentFamilyIndex, aOther.presentFamilyIndex );
		std::swap( presentQueue, aOther.presentQueue );
		std::swap( swapchain, aOther.swapchain );
		std::swap( swapImages, aOther.swapImages );
		std::swap( swapViews, aOther.swapViews );
		std::swap( swapchainFormat, aOther.swapchainFormat );
		std::swap( swapchainExtent, aOther.swapchainExtent );
		return *this;
	}

	// makeVulkanWindow()
	VulkanWindow makeVulkanWindow(Kiki::WindowInfo info) {
		VulkanWindow ret;

		#ifdef __APPLE__
			setenv("MVK_CONFIG_SYNCHRONOUS_QUEUE_SUBMITS", "0", 1);
			setenv("MVK_CONFIG_PRESENT_WITH_COMMAND_BUFFER", "1", 1);
		#endif

		// Initialize Volk
		if( auto const res = volkInitialize(); VK_SUCCESS != res ) {
			throw Kiki::FatalError( "Unable to load Vulkan API\n" 
				"Volk returned error {}", toString(res)
			);
		}

		// Initialize GLFW and make sure this GLFW supports Vulkan.
        // Note: this assumes that we will not create multiple windows that exist concurrently. If multiple windows are
        // to be used, the glfwInit() and the glfwTerminate() (see destructor) calls should be moved elsewhere.
        if( GLFW_TRUE != glfwInit() ) {
            char const* errMsg = nullptr;
            glfwGetError( &errMsg );
            
            throw Kiki::FatalError( "GLFW initialization failed: {}", errMsg );
        }
        
        if( !glfwVulkanSupported() ) {
            throw Kiki::FatalError( "GLFW: Vulkan not supported." );
        }

		// Check for instance layers and extensions
		auto const supportedLayers = getInstanceLayers();
		auto const supportedExtensions = getInstanceExtensions();

		bool enableDebugUtils = false;

		std::vector<char const*> enabledLayers, enabledExensions;

		// GLFW may require a number of instance extensions
        // GLFW returns a bunch of pointers-to-strings; however, GLFW manages these internally, so we must not
        // free them ourselves. GLFW guarantees that the strings remain valid until GLFW terminates.
        std::uint32_t reqExtCount = 0;
        char const** requiredExt = glfwGetRequiredInstanceExtensions( &reqExtCount );

        for( std::uint32_t i = 0; i < reqExtCount; ++i ) {
            if( !supportedExtensions.count( requiredExt[i] ) ) {
                throw Kiki::FatalError( "GLFW/Vulkan: required instance extension {} not supported", requiredExt[i] );
            }

            enabledExensions.emplace_back( requiredExt[i] );
        }

        // For Mac compatibility
#		ifdef __APPLE__
		enabledExensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
#		endif

		// Validation layers support.
#		if !defined(NDEBUG) // debug builds only
		if( supportedLayers.count( "VK_LAYER_KHRONOS_validation" ) )
		{
			enabledLayers.emplace_back( "VK_LAYER_KHRONOS_validation" );
		}

		if( supportedExtensions.count( "VK_EXT_debug_utils" ) )
		{
			enableDebugUtils = true;
			enabledExensions.emplace_back( "VK_EXT_debug_utils" );
		}
#		endif // ~ debug builds

		for( auto const& layer : enabledLayers )
			std::print( stderr, "Enabling layer: {}\n", layer );

		for( auto const& extension : enabledExensions )
			std::print( stderr, "Enabling instance extension: {}\n", extension );

		// Create Vulkan instance
		ret.instance = createInstance( enabledLayers, enabledExensions, enableDebugUtils );

		// Load rest of the Vulkan API
		volkLoadInstance( ret.instance );

		// Setup debug messenger
		if( enableDebugUtils )
			ret.debugMessenger = createDebugMessenger(ret.instance);

		// Create GLFW Window and the Vulkan surface
        glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );

		// Get details of the monitor
		int count;
		GLFWmonitor** monitors = glfwGetMonitors(&count);
		GLFWmonitor* monitor;

		if (info.monitor < count && info.monitor >= 0) {
			monitor = monitors[info.monitor];
		} else {
			monitor = glfwGetPrimaryMonitor();
			if (monitor == NULL) {
				throw Kiki::FatalError("Invalid monitor index given & no primary monitor found");
			}
		}

		if (!monitor) {
            char const* errMsg = nullptr;
            glfwGetError(&errMsg);

            throw Kiki::FatalError( "Unable to get monitor\n"
                "Last error = {}", errMsg
            );
        }

		const GLFWvidmode* videoMode = glfwGetVideoMode(monitor);
		if (!videoMode) {
            char const* errMsg = nullptr;
            glfwGetError(&errMsg);

            throw Kiki::FatalError( "Unable to get monitor's video mode\n"
                "Last error = {}", errMsg
            );
        }

		// Check if the width/height is 0, and if so set to monitors resolution
		if (info.width == 0) {
			info.width = videoMode->width;
		}

		if (info.height == 0) {
			info.height = videoMode->height;
		}

		// Check if decorations are disabled
		if (!info.decorations) {
			glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
		}

		// Create window (and check if it needs to be fullscreen)
		if (info.fullscreen) {
			ret.window = glfwCreateWindow(info.width, info.height, info.title, monitor, nullptr);
		} else {
        	ret.window = glfwCreateWindow(info.width, info.height, info.title, nullptr, nullptr);
		}
        if( !ret.window ) {
            char const* errMsg = nullptr;
            glfwGetError( &errMsg );

            throw Kiki::FatalError( "Unable to create GLFW window\n"
                "Last error = {}", errMsg
            );
        }

		// Check if resizing is disabled
		if (!info.resizeable) {
			glfwSetWindowSizeLimits(ret.window, info.width, info.height, info.width, info.height);
		}

        if( auto const res = glfwCreateWindowSurface( ret.instance, ret.window, nullptr, &ret.surface ); VK_SUCCESS != res ) {
            throw Kiki::FatalError( "Unable to create VkSurfaceKHR\n"
                "glfwCreateWindowSurface() returned {}", rutils::toString(res)
            );
        }

		// Select appropriate Vulkan device
		ret.physicalDevice = rutils::selectDevice( ret.instance, ret.surface );
		if( VK_NULL_HANDLE == ret.physicalDevice )
			throw Kiki::FatalError( "No suitable physical device found!" );

		{
			VkPhysicalDeviceProperties props;
			vkGetPhysicalDeviceProperties( ret.physicalDevice, &props );
			std::print( stderr, "Selected device: {} ({}.{}.{})\n", props.deviceName, VK_API_VERSION_MAJOR(props.apiVersion), VK_API_VERSION_MINOR(props.apiVersion), VK_API_VERSION_PATCH(props.apiVersion) );
		}

		// Create a logical device
		// Enable required extensions. The device selection method ensures that
		// the VK_KHR_swapchain extension is present, so we can safely just
		// request it without further checks.
		std::vector<char const*> enabledDevExensions;

		enabledDevExensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        // For mac support
#		ifdef __APPLE__
        enabledDevExensions.emplace_back("VK_KHR_portability_subset");
#		endif

		for( auto const& ext : enabledDevExensions )
			std::print( stderr, "Enabling device extension: {}\n", ext );

		// We need one or two queues:
		// - best case: one GRAPHICS queue that can present
		// - otherwise: one GRAPHICS queue and any queue that can present
		std::vector<std::uint32_t> queueFamilyIndices;

		if( auto const index = rutils::findQueueFamily( ret.physicalDevice, VK_QUEUE_GRAPHICS_BIT, ret.surface ) ) {
            ret.graphicsFamilyIndex = *index;

            queueFamilyIndices.emplace_back( *index );
        }
        else {
            auto graphics = rutils::findQueueFamily( ret.physicalDevice, VK_QUEUE_GRAPHICS_BIT ); 
            auto present = rutils::findQueueFamily( ret.physicalDevice, 0, ret.surface );

            assert( graphics && present );

            ret.graphicsFamilyIndex = *graphics;
            ret.presentFamilyIndex = *present;

            queueFamilyIndices.emplace_back( *graphics );
            queueFamilyIndices.emplace_back( *present );
        }

		ret.device = rutils::createDevice(ret.physicalDevice, queueFamilyIndices, enabledDevExensions);

		// Retrieve VkQueues
		vkGetDeviceQueue( ret.device, ret.graphicsFamilyIndex, 0, &ret.graphicsQueue );

		assert( VK_NULL_HANDLE != ret.graphicsQueue );

		if( queueFamilyIndices.size() >= 2 )
			vkGetDeviceQueue( ret.device, ret.presentFamilyIndex, 0, &ret.presentQueue );
		else
		{
			ret.presentFamilyIndex = ret.graphicsFamilyIndex;
			ret.presentQueue = ret.graphicsQueue;
		}

		// Create swap chain
		std::tie(ret.swapchain, ret.swapchainFormat, ret.swapchainExtent) = createSwapchain( ret.physicalDevice, ret.surface, ret.device, ret.window, queueFamilyIndices );
		
		// Get swap chain images & create associated image views
		getSwapchainImages( ret.device, ret.swapchain, ret.swapImages );
		createSwapchainImageViews( ret.device, ret.swapchainFormat, ret.swapImages, ret.swapViews );

		// Done
		return ret;
	}

	SwapChanges recreateSwapchain( VulkanWindow& aWindow ) {
		// Remember old format & extents
		// These are two of the properties that may change. Typically only the extent changes (e.g., window resized),
		// but the format may in theory also change. If the format changes, we need to recreate additional resources.
		auto const oldFormat = aWindow.swapchainFormat;
		auto const oldExtent = aWindow.swapchainExtent;

		// Destroy old objects (except for the old swap chain)
		// We keep the old swap chain object around, such that we can pass it to vkCreateSwapchainKHR() via the
		// oldSwapchain member of VkSwapchainCreateInfoKHR.
		VkSwapchainKHR oldSwapchain = aWindow.swapchain;

		for (auto view : aWindow.swapViews)
		vkDestroyImageView( aWindow.device, view, nullptr );

		aWindow.swapViews.clear();
		aWindow.swapImages.clear();

		// Create swap chain
		std::vector<std::uint32_t> queueFamilyIndices;
		if (aWindow.presentFamilyIndex != aWindow.graphicsFamilyIndex) {
			queueFamilyIndices.emplace_back( aWindow.graphicsFamilyIndex );
			queueFamilyIndices.emplace_back( aWindow.presentFamilyIndex );
		}

		try {
			std::tie(aWindow.swapchain, aWindow.swapchainFormat, aWindow.swapchainExtent) = createSwapchain(aWindow.physicalDevice, aWindow.surface, aWindow.device, aWindow.window, queueFamilyIndices, oldSwapchain);
		}
		catch( ... ) {
			// Put pack the old swap chain handle into the VulkanWindow; this ensures that the old swap chain is
			// destroyed when this error branch occurs.
			aWindow.swapchain = oldSwapchain;
			throw;
		}

		// Destroy old swap chain
		vkDestroySwapchainKHR( aWindow.device, oldSwapchain, nullptr );

		// Get new swap chain images & create associated image views
		getSwapchainImages( aWindow.device, aWindow.swapchain, aWindow.swapImages );
		createSwapchainImageViews( aWindow.device, aWindow.swapchainFormat, aWindow.swapImages, aWindow.swapViews );

		// Determine which swap chain properties have changed and return the information indicating this
		SwapChanges ret{};

		if (oldExtent.width != aWindow.swapchainExtent.width || oldExtent.height != aWindow.swapchainExtent.height)
			ret.changedSize = true;
		if (oldFormat != aWindow.swapchainFormat)
			ret.changedFormat = true;

		return ret;
	}
}

std::unordered_set<std::string> getInstanceLayers() {
    std::uint32_t numLayers = 0;
	if( auto const res = vkEnumerateInstanceLayerProperties( &numLayers, nullptr ); VK_SUCCESS != res ) {
		throw Kiki::FatalError( "Unable to enumerate layers\n"
			"vkEnumerateInstanceLayerProperties() returned {}\n", rutils::toString(res)
		);
	}

	std::vector<VkLayerProperties> layers( numLayers );
	if( auto const res = vkEnumerateInstanceLayerProperties( &numLayers, layers.data() ); VK_SUCCESS != res ) {
		throw Kiki::FatalError( "Unable to get layer properties\n"
			"vkEnumerateInstanceLayerProperties() returned {}", rutils::toString(res)
		);
	}

	std::unordered_set<std::string> res;
	for( auto const& layer : layers )
		res.insert( layer.layerName );

	return res;
}

std::unordered_set<std::string> getInstanceExtensions() {
	std::uint32_t numExtensions = 0;
	if( auto const res = vkEnumerateInstanceExtensionProperties( nullptr, &numExtensions, nullptr ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to enumerate extensions\n"
			"vkEnumerateInstanceExtensionProperties() returned {}", rutils::toString(res)
		);
	}

	std::vector<VkExtensionProperties> extensions( numExtensions );
	if( auto const res = vkEnumerateInstanceExtensionProperties( nullptr, &numExtensions, extensions.data() ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to get extension properties\n" 
			"vkEnumerateInstanceExtensionProperties() returned {}", rutils::toString(res)
		);
	}

	std::unordered_set<std::string> res;
	for( auto const& extension : extensions )
		res.insert( extension.extensionName );

	return res;
}

VkInstance createInstance(std::vector<char const*> const& aEnabledLayers, std::vector<char const*> const& aEnabledExtensions, bool aEnableDebugUtils) {
    // Prepare debug messenger info
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};

    if( aEnableDebugUtils )
    {
        debugInfo.sType  = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugInfo.messageSeverity  = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        debugInfo.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugInfo.pfnUserCallback  = &debug_util_callback;
        debugInfo.pUserData        = nullptr;
    }

    // Prepare application info
    // The `apiVersion` is the *highest* version of Vulkan than the
    // application can use. We can therefore safely set it to 1.3, even if
    // we are not intending to use any 1.3 features (and want to run on
    // pre-1.3 implementations).
    VkApplicationInfo appInfo{};
    appInfo.sType  = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName    = "kiki";
    appInfo.applicationVersion  = 1;
    appInfo.apiVersion          = VK_MAKE_API_VERSION( 0, 1, 4, 0 ); // Version 1.4

    // Create instance
    VkInstanceCreateInfo instanceInfo{};
    instanceInfo.sType  = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    instanceInfo.enabledLayerCount        = std::uint32_t(aEnabledLayers.size());
    instanceInfo.ppEnabledLayerNames      = aEnabledLayers.data();

    instanceInfo.enabledExtensionCount    = std::uint32_t(aEnabledExtensions.size());
    instanceInfo.ppEnabledExtensionNames  = aEnabledExtensions.data();

#	ifdef __APPLE__
    instanceInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#	endif

    instanceInfo.pApplicationInfo = &appInfo;

    if( aEnableDebugUtils ) {
        debugInfo.pNext = instanceInfo.pNext;
        instanceInfo.pNext = &debugInfo; 
    }

    VkInstance instance;
    if( auto const res = vkCreateInstance( &instanceInfo, nullptr, &instance ); VK_SUCCESS != res ) {
        throw Kiki::FatalError( "Unable to create Vulkan instance\n"
            "vkCreateInstance() returned {}", rutils::toString(res)
        );
    }

    return instance;
}

VkDebugUtilsMessengerEXT createDebugMessenger(VkInstance aInstance) {
    // Set up the debug messaging for the rest of the application
    VkDebugUtilsMessengerCreateInfoEXT debugInfo{};
    debugInfo.sType            = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugInfo.messageSeverity  = /*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT | */ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugInfo.messageType      = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugInfo.pfnUserCallback  = &debug_util_callback;
    debugInfo.pUserData        = nullptr;

    VkDebugUtilsMessengerEXT messenger = VK_NULL_HANDLE;
    if( auto const res = vkCreateDebugUtilsMessengerEXT( aInstance, &debugInfo, nullptr, &messenger ); VK_SUCCESS != res ) {
        throw Kiki::FatalError( "Unable to set up debug messenger\n" 
            "vkCreateDebugUtilsMessengerEXT() returned {}", rutils::toString(res)
        );
    }

    return messenger;
}

VKAPI_ATTR VkBool32 VKAPI_CALL debug_util_callback( VkDebugUtilsMessageSeverityFlagBitsEXT aSeverity, VkDebugUtilsMessageTypeFlagsEXT aType, VkDebugUtilsMessengerCallbackDataEXT const* aData, void* /*aUserPtr*/ ) {
    std::print( stderr, "{} ({}): {} ({})\n{}\n--\n", rutils::toString(aSeverity), rutils::messageTypeFlags(aType), aData->pMessageIdName, aData->messageIdNumber, aData->pMessage );

    return VK_FALSE;
}

std::tuple<VkSwapchainKHR,VkFormat,VkExtent2D> createSwapchain(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface, VkDevice aDevice, GLFWwindow* aWindow, std::vector<std::uint32_t> const& aQueueFamilyIndices, VkSwapchainKHR aOldSwapchain) {
    auto const formats = getSurfaceFormats( aPhysicalDev, aSurface );
    auto const modes = getPresentModes( aPhysicalDev, aSurface );

    // Pick the surface format
    // If there is an 8-bit RGB(A) SRGB format available, pick that. There are two main variations possible here
    // RGBA and BGRA. If neither is available, pick the first one that the driver gives us.
    //
    // See http://vulkan.gpuinfo.org/listsurfaceformats.php for a list of formats and statistics about where they’re
    // supported.
    assert (!formats.empty());

    VkSurfaceFormatKHR format = formats[0];
    for (auto const fmt : formats) {
        if (VK_FORMAT_R8G8B8A8_SRGB == fmt.format && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == fmt.colorSpace) {
            format = fmt;
            break;
        }

        if (VK_FORMAT_B8G8R8A8_SRGB == fmt.format && VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == fmt.colorSpace) {
            format = fmt;
            break;
        }
    }

    // Pick a presentation mode
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

    // Prefer MAILBOX if it’s available.
    if (modes.count(VK_PRESENT_MODE_MAILBOX_KHR))
        presentMode = VK_PRESENT_MODE_MAILBOX_KHR;

    // Pick an image count
    VkSurfaceCapabilitiesKHR caps;
    if (auto const res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aPhysicalDev, aSurface, &caps); VK_SUCCESS != res) {
        throw Kiki::FatalError( "Unable to get surface capabilities\n"
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR() returned {}", rutils::toString(res)
        );
    }

    std::uint32_t imageCount = 2;

    if (imageCount < caps.minImageCount + 1)
        imageCount = caps.minImageCount + 1;

    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount)
        imageCount = caps.maxImageCount;

    // Figure out the swap extent
    VkExtent2D extent = caps.currentExtent;
    if (std::numeric_limits<std::uint32_t>::max() == extent.width) {
        int width, height;
        glfwGetFramebufferSize( aWindow, &width, &height );

        // Note: we must ensure that the extent is within the range defined by [ minImageExtent, maxImageExtent ].
        auto const& min = caps.minImageExtent;
        auto const& max = caps.maxImageExtent;

        extent.width = std::clamp( std::uint32_t(width), min.width, max.width );
        extent.height = std::clamp( std::uint32_t(height), min.height, max.height );
    }

    // Finally create the swap chain
    VkSwapchainCreateInfoKHR chainInfo{};
    chainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    chainInfo.surface = aSurface;
    chainInfo.minImageCount = imageCount;
    chainInfo.imageFormat = format.format;
    chainInfo.imageColorSpace = format.colorSpace;
    chainInfo.imageExtent = extent;
    chainInfo.imageArrayLayers = 1;
    chainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    chainInfo.preTransform = caps.currentTransform;
    chainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    chainInfo.presentMode = presentMode;
    chainInfo.clipped = VK_TRUE;
    chainInfo.oldSwapchain = aOldSwapchain;

    if(aQueueFamilyIndices.size() <= 1){
        chainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    else {
        // Multiple queues may access this resource. There are two options. SHARING MODE CONCURRENT
        // allows access from multiple queues without transferring ownership. EXCLUSIVE would require explicit
        // ownership transfers, which we’re avoiding for now. EXCLUSIVE may result in better performance than
        // CONCURRENT.
        chainInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        chainInfo.queueFamilyIndexCount = std::uint32_t(aQueueFamilyIndices.size());
        chainInfo.pQueueFamilyIndices = aQueueFamilyIndices.data();
    }

    VkSwapchainKHR chain = VK_NULL_HANDLE;
    if (auto const res = vkCreateSwapchainKHR( aDevice, &chainInfo, nullptr, &chain); VK_SUCCESS != res) {
        throw Kiki::FatalError( "Unable to create swap chain\n"
            "vkCreateSwapchainKHR() returned {}", rutils::toString(res)
        );
    }

    return { chain, format.format, extent };
}


void getSwapchainImages(VkDevice aDevice, VkSwapchainKHR aSwapchain, std::vector<VkImage>& aImages) {
    assert( 0 == aImages.size() );

    std::uint32_t numImages = 0;
	if( auto const res = vkGetSwapchainImagesKHR( aDevice, aSwapchain, &numImages, nullptr ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to enumerate images\n"
			"vkGetSwapchainImagesKHR() returned {}", rutils::toString(res)
		);
	}

	aImages.resize(numImages);
	if( auto const res = vkGetSwapchainImagesKHR( aDevice, aSwapchain, &numImages, aImages.data() ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to get images\n" 
			"vkGetSwapchainImagesKHR() returned {}", rutils::toString(res)
		);
	}
}

void createSwapchainImageViews(VkDevice aDevice, VkFormat aSwapchainFormat, std::vector<VkImage> const& aImages, std::vector<VkImageView>& aViews) {
    assert( 0 == aViews.size() );

    for (std::size_t i = 0; i < aImages.size(); ++i) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = aImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = aSwapchainFormat;
        viewInfo.components = VkComponentMapping{
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY,
            VK_COMPONENT_SWIZZLE_IDENTITY
        };
        viewInfo.subresourceRange = VkImageSubresourceRange{
            VK_IMAGE_ASPECT_COLOR_BIT,
            0, 1,
            0, 1
            };

        VkImageView view = VK_NULL_HANDLE;
        if (auto const res = vkCreateImageView( aDevice, &viewInfo, nullptr, &view ); VK_SUCCESS != res) {
            throw Kiki::FatalError( "Unable to create image view for swap chain image {}\n"
                "vkCreateImageView() returned {}", i, rutils::toString(res)
            );
        }

        aViews.emplace_back(view);
    }

    assert( aViews.size() == aImages.size() );
}

std::vector<VkSurfaceFormatKHR> getSurfaceFormats(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface) {
    std::uint32_t numFormats = 0;
	if( auto const res = vkGetPhysicalDeviceSurfaceFormatsKHR( aPhysicalDev, aSurface, &numFormats, nullptr ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to enumerate surface formats\n"
			"vkGetPhysicalDeviceSurfaceFormatsKHR() returned {}", rutils::toString(res)
		);
	}

	std::vector<VkSurfaceFormatKHR> formats( numFormats );
	if( auto const res = vkGetPhysicalDeviceSurfaceFormatsKHR( aPhysicalDev, aSurface, &numFormats, formats.data() ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to get surface formats\n" 
			"vkGetPhysicalDeviceSurfaceFormatsKHR() returned {}", rutils::toString(res)
		);
	}

	return formats;
}

std::unordered_set<VkPresentModeKHR> getPresentModes(VkPhysicalDevice aPhysicalDev, VkSurfaceKHR aSurface) {
    std::uint32_t numModes = 0;
	if( auto const res = vkGetPhysicalDeviceSurfacePresentModesKHR( aPhysicalDev, aSurface, &numModes, nullptr ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to enumerate presentation modes\n"
			"vkGetPhysicalDeviceSurfaceFormatsKHR() returned {}", rutils::toString(res)
		);
	}

	std::vector<VkPresentModeKHR> modes( numModes );
	if( auto const res = vkGetPhysicalDeviceSurfacePresentModesKHR( aPhysicalDev, aSurface, &numModes, modes.data() ); VK_SUCCESS != res ) {	
		throw Kiki::FatalError( "Unable to get presentation modes\n" 
			"vkGetPhysicalDeviceSurfacePresentModesKHR() returned {}", rutils::toString(res)
		);
	}

    std::unordered_set<VkPresentModeKHR> res;
	for( auto const& mode : modes )
		res.insert( mode );

	return res;
}