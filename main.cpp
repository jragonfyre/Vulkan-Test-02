
//#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>
#include <cstdlib>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <optional>
#include <cstring>
#include <cstdint>
#include <algorithm>
#include <glm/glm.hpp>
#include <array>


#include "utils.h"
#include "JShaderModule.h"
#include "JBuffer.h"


const constexpr uint32_t WIDTH = 800;
const constexpr uint32_t HEIGHT = 600;
const constexpr int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> validationLayers = {
	"VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};


#ifdef NDEBUG
	const bool enableValidationLayers = false;
#else
	const bool enableValidationLayers = true;
#endif

// validation layer support check
bool checkValidationLayerSupport() {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers) {
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) {
			if (strcmp(layerName, layerProperties.layerName) == 0) {
				layerFound = true;
				break;
			}
		}
		if (!layerFound) {
			return false; // this layer was not found
		}
	}
	return true; // all layers found
}

// returns required extensions depending on whether validation layers are enabled or not
std::vector<const char*> getRequiredExtensions() {
	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
	std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

	if (enableValidationLayers) {
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

// for storing the QueueFamilyIndices we care about
struct QueueFamilyIndices {
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete() {
		return graphicsFamily.has_value() && presentFamily.has_value();
	}
};

// for storing details of swapchain support
struct SwapChainSupportDetails {
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;
};



// proxy function to lookup address of create debug utils messenger extension function
// extensions are not loaded by default
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
	const VkAllocationCallbacks* pAllocator,
	VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) { // nullptr if function couldn't be found
		return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}
// proxy function to lookup address of destroy debug utils messenger extension function
// extensions are not loaded by default
void DestroyDebugUtilsMessengerEXT(VkInstance instance,
	const VkDebugUtilsMessengerEXT debugMessenger,
	const VkAllocationCallbacks* pAllocator) {
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) { // nullptr if function couldn't be found
		func(instance, debugMessenger, pAllocator);
	}
	else {
		std::cerr << "failed to load destory debug utils messenger function" << std::endl;
	}
}


struct Vertex {
	glm::vec2 pos;
	glm::vec3 color;

	static VkVertexInputBindingDescription getBindingDescription() {
		VkVertexInputBindingDescription bindingDescription{}; // how is the data packed?
		bindingDescription.binding = 0; // one vertex data array, this is index 0
		bindingDescription.stride = sizeof(Vertex); // number of bytes from one entry to the next
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		// the other option is INSTANCE, move to next data entry after each VERTEX/INSTANCE
		// instanced rendering is for making a bunch of copies of a model

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
		std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
		// how to extract a vertex attribute from a chunk of vertex data originating from a binding 
		// description
		// two attributes, pos and color, so we need two attribute description structs
		attributeDescriptions[0].binding = 0; // which binding is the per-vertex data from
		attributeDescriptions[0].location = 0; // same location as shader
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // two 32 bit floats, i.e. vec2
		// for some reason, same as color formats
		attributeDescriptions[0].offset = offsetof(Vertex, pos); // calculate offset of pos in struct

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		return attributeDescriptions;
	}
};

const std::vector<Vertex> vertices = {
	{{0.0f, -0.5f}, {1.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}}
};


class HelloTriangleApplication {
public:
	void run() {
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}
private:
	// data members
	GLFWwindow* window; // window ptr
	VkInstance instance; // Instance handle
	VkDebugUtilsMessengerEXT debugMessenger; // handle for debug callback
	VkSurfaceKHR surface;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // implicitly destroyed when instance is
	
	VkDevice device; // logical device
	// queues
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	
	// swapchain stuff
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	
	// image views
	std::vector<VkImageView> swapChainImageViews;
	
	// render passes 
	VkRenderPass renderPass;
	// pipeline stuff
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline; 

	// framebuffers
	std::vector<VkFramebuffer> swapChainFramebuffers;

	// command pools and buffers
	VkCommandPool commandPool;
	std::vector<VkCommandBuffer> commandBuffers;

	// drawing stuff
	std::vector<VkSemaphore> imageAvailableSemaphores;
	std::vector<VkSemaphore> renderFinishedSemaphores;
	std::vector<VkFence> inFlightFences;
	std::vector<VkFence> imagesInFlight;
	size_t currentFrame = 0;

	bool framebufferResized = false;

	// vertex buffer
	JBuffer* vertBuffer = nullptr;
	
	//VkBuffer vertexBuffer;
	//VkDeviceMemory vertexBufferMemory;


	// member functions
	void initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); now we're handling resizes properly

		window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
		glfwSetWindowUserPointer(window, this); // give window a pointer to this
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
		auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
		app->framebufferResized = true;
	}

	void initVulkan() {
		createInstance();
		setupDebugMessenger();
		createSurface();
		pickPhysicalDevice();
		createLogicalDevice();
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandPool();
		createVertexBuffer();
		createCommandBuffers();
		createSyncObjects();
	}
	void createInstance() {
		// do a preliminary check for validation layers
		if (enableValidationLayers && !checkValidationLayerSupport()) {
			throw std::runtime_error("validation layers requested, but not available!");
		}
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Hello Triangle";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "No Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;


		// extensions 

		/*uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		*/

		auto extensions = getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		// enumerate available extensions for fun
		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());

		std::cout << "available extensions:\n";

		for (const auto& extension : availableExtensions) {
			std::cout << "\t" << extension.extensionName << "\n";
		}

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		// validation layers
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			// set up debug callback to be called on error in vkCreateInstance by setting up 
			// pNext to be a pointer to the debugCreateInfo
			// also gets used in vkDestroyInstance and then cleaned up automatically
			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;

			createInfo.pNext = nullptr;
		}

		// object creation pattern:
		// pointer to struct with creation info
		// pointer to custom allocator callbacks, nullptr for now
		// pointer to variable that stores the handle to the new object
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
			throw std::runtime_error("failed to create Vulkan instance!");
		}
		else {
			std::cerr << "Vulkan created successfully" << std::endl;
		}
	}
	void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		// severities we want callback to be called for
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		// verbose, warning, error, no info though
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
			| VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
		// enable all message types
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // optional
	}
	
	void setupDebugMessenger() {
		if (!enableValidationLayers) return;
		VkDebugUtilsMessengerCreateInfoEXT createInfo;
		populateDebugMessengerCreateInfo(createInfo);

		if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
			throw std::runtime_error("failed to set up debug messenger!");
		}
	}

	void createSurface() {
		// uses glfw builtin function rather than populating a pile of structs
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface!");
		}
	}

	void pickPhysicalDevice() {
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
		if (deviceCount == 0) {
			throw std::runtime_error("failed to find GPUs with Vulkan support!");
		}
		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
		
		// sort candidates by score using multimap
		std::multimap<int, VkPhysicalDevice> candidates;

		for (const auto& device : devices) {
			int score = rateDeviceSuitability(device);
			candidates.insert(std::make_pair(score, device));
		}
		if (candidates.rbegin()->first > 0) {
			physicalDevice = candidates.rbegin()->second;
		}
		else {
			throw std::runtime_error("failed to find a suitable GPU!");
		}

	}

	bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}
		return requiredExtensions.empty();
	}

	/*
	bool isDeviceSuitable(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties; // device properties
		VkPhysicalDeviceFeatures deviceFeatures; // optional features

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);


		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
			&& deviceFeatures.geometryShader;
	}
	*/

	int rateDeviceSuitability(VkPhysicalDevice device) {
		VkPhysicalDeviceProperties deviceProperties; // device properties
		VkPhysicalDeviceFeatures deviceFeatures; // optional features
		QueueFamilyIndices indices = findQueueFamilies(device);

		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		int score = 0;

		// discrete gpus have performance advantage over integrated
		if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
			score += 1000;
		}
		else {
			score += 10;
		}

		if (!indices.isComplete()) {
			return 0; // must have a graphics family
		}

		bool extensionsSupported = checkDeviceExtensionSupport(device);
		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
			// check that we have at least one format and present mode
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		if (!swapChainAdequate) {
			return 0; // can't use if we don't have adequate swap chain support
		}

		return score;

		// example stuff

		// max texture size affects graphics quality
		score += deviceProperties.limits.maxImageDimension2D;

		// can't function without geometry shaders
		if (!deviceFeatures.geometryShader) {
			return 0;
		}
		return score;
	}

	QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
		QueueFamilyIndices indices;

		// assign index to queue families that could be found
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		int i = 0;
		for (const auto& queueFamily : queueFamilies) {
			// check for graphics support
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
				indices.graphicsFamily = i;
			}
			// check for presenting to our window surface support
			// could add logic to check to see if the queue supports graphics and presentation for efficiency
			// but it doesn't really matter if they are the same for us
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
			if (presentSupport) {
				indices.presentFamily = i;
			}

			if (indices.isComplete()) {
				break;
			}

			++i;
		}

		return indices;
	}

	SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) const {
		SwapChainSupportDetails details;
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

		if (formatCount != 0) {
			details.formats.resize(formatCount); // resize vector to hold all format counts
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

		if (presentModeCount != 0) {
			details.presentModes.resize(presentModeCount); // resize vector to hold all format counts
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}

	VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
				&& availableFormat.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR) {
				return availableFormat;
			}
		}

		// eh whatever, first is probably fine if the preferred is not available
		return availableFormats[0];
	}
	VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
		for (const auto& presentMode : availablePresentModes) {
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) { // used for triple buffering
				return presentMode;
			}
		}

		return VK_PRESENT_MODE_FIFO_KHR; // guaranteed to be available, ordinary vsync
	}

	// resolution of swap chain images
	// almost always exactly equal to window resolution
	// but some window managers allow a difference, indicated by setting width and height to max value 
	// of a uint32_t
	VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) { // need <cstdint> include for UINT32_MAX
			return capabilities.currentExtent; // modification not allowed
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = { 
				static_cast<uint32_t>(width), 
				static_cast<uint32_t>(height) 
			};

			actualExtent.width = std::max(capabilities.minImageExtent.width,
				std::min(capabilities.maxImageExtent.width, actualExtent.width));
			actualExtent.height = std::max(capabilities.minImageExtent.height,
				std::min(capabilities.maxImageExtent.height, actualExtent.height));

			return actualExtent;
		}
	}

	void createLogicalDevice() {
		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

		// vector of queue infos
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		// set of distinct queue indices
		std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// don't need any device features yet, so leave blank
		VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pEnabledFeatures = &deviceFeatures;

		//createInfo.enabledExtensionCount = 0; // device extensions, none for now
		createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = deviceExtensions.data();


		// not needed for modern vulkan, but for compatibility, include validation layers on device createinfo
		if (enableValidationLayers) {
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		// create the logical device
		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
			throw std::runtime_error("failed to create logical device!");
		}

		// finally retrieve the queue handles (the queues are created with the device)
		// 0 is the index of the queue in the family, since we're only creating one.
		vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
	}

	void createSwapChain() {
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 // no maximum
			&& imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount; // set to max if we want more than max
		}

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // number of layers per image (1 unless doing stereoscopic 3D)
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 

		QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };
		if (indices.graphicsFamily != indices.presentFamily) {
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT; // shared across queues
			createInfo.queueFamilyIndexCount = 2; 
			createInfo.pQueueFamilyIndices = queueFamilyIndices;

			// don't want to deal with ownership transfers right now
		}
		else {
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE; // not shared across queues
			//optional:
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
			// this works because the indices are the same, no sharing necessary
		}
		
		createInfo.preTransform = swapChainSupport.capabilities.currentTransform; // no transformation applied
		// to images in swap chain.
		// don't blend with other windows in window system
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; // don't care about color of obscured pixels.

		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
			throw std::runtime_error("failed to create swap chain!");
		}

		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
		swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

		// save chosen format and extent for later use
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}
	
	void createImageViews() {
		swapChainImageViews.resize(swapChainImages.size());

		for (size_t i = 0; i < swapChainImages.size(); ++i) {
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = swapChainImages[i];
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // texture 2d essentially
			createInfo.format = swapChainImageFormat;

			// default channel mapping
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
			
			// subresourceRange describes the image's purpose and which part should be accessed, no mipmap levels or layers
			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1; // for stereographic app might want two layers and then two image views 
			// one for each layer
			if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create image views!");
			}
		}
	}

	void createRenderPass() {
		// one color buffer attachment, rep by one image in swap chain
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat; // match swap chain images format
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling right now
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear values to a constant at start
		// other ops are LOAD (preserve existing) DONT_CARE (undefined, we don't care)
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // two possibilities
		// STORE (store in memory, can be read), DONT_CARE (undefined after the rendering operation)
		
		// stencil data
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // dont care about stencil data
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		
		// layout
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // we're going to clear it anyway, don't care initial layout
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // want to be ready for presentation after rendering
		// common layout options:
		// COLOR_ATTACHMENT_OPTIMAL: image used as color attachment
		// PRESENT_SRC_KHR: image to be presented in swap chain
		// TRANSFER_DST_OPTIMAL: used as destination for a memory copy operation

		// Subpasses:
		// attachment ref for subpass
		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0; // which attachment to reference by index in attach descriptions array
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // best layout for a color buffer

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // alternatively compute
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		// other things that can be referenced,
		// pInputAttachments
		// pResolveAttachments
		// pDepthStencilAttachment
		// pPreserveAttachments (not used by subpass, but data must be preserved)

		// subpass dependencies
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		// VK_SUBPASS_EXTERNAL means implicit subpass before/after render pass 
		// depending on whether it's in source or dest
		// index 0 refers to our subpass, dst must be > src to prevent cycles in the graph
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // ops to wait on 
		dependency.srcAccessMask = 0; 

		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		// subpass dependencies
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
			throw std::runtime_error("failed to create render pass!");
		}
	}

	void createGraphicsPipeline() {
		//auto vertShaderCode = readFile("shaders/vert.spv");
		//auto fragShaderCode = readFile("shaders/frag.spv");

		// shader modules are compiled and linked when the pipeline is created, so we can destroy them after that
		//VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		//VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		JShaderModule vertModule(device, JShaderType::JVertex, "shaders/vert.spv");
		JShaderModule fragModule(device, JShaderType::JFragment, "shaders/frag.spv");

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo = vertModule.stageInfo();
		/*
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // vertex shader stage
		vertShaderStageInfo.module = vertModule.module(); //vertShaderModule;
		vertShaderStageInfo.pName = "main"; // entry point
		// not usng pSpecializationInfo, but you can pass in compile time constants, and compiler can optimize 
		// with these values, we're setting it to nullptr
		*/
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo = fragModule.stageInfo();
		/*
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT; // vertex shader stage
		fragShaderStageInfo.module = fragModule.module(); //fragShaderModule;
		fragShaderStageInfo.pName = "main"; // entry point
		*/

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();

		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		// input assembly describes topology info
		// point list, line list, line strip, triangle list, triangle strip etc
		// primitive restart enable allows reseting a strip topology in the middle with indices 0xFFFF or 0xFFFFFFFF
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// set up viewport (region of framebuffer to output to)
		// usually (0,0) to (width,height)
		// viewport is transformation from image to framebuffer, so changing this would rescale, not clip
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// scissors clip where we draw
		// want to draw to the whole framebuffer, so:
		VkRect2D scissor{};
		scissor.offset = { 0,0 };
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1; // more than one needs a GPU feature
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// rasterizer configuration
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // discard fragments that are too far away
		// setting to true requires a GPU feature, and can be useful for things like shadow maps
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true, geometry never passes through rasterizer
		// no framebuffer output
		// polygonMode options: FILL, LINE, POINT doing the expected, anything other than fill needs a 
		// GPU feature
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f; // anything wider than 1.0f requires wideLines GPU feature
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // cull back faces
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE; // front faces are clockwise
		rasterizer.depthBiasEnable = VK_FALSE; // sometimes used for shadow mapping (bias based on slope)
		// optional:
		rasterizer.depthBiasConstantFactor = 0.0f;
		rasterizer.depthBiasClamp = 0.0f;
		rasterizer.depthBiasSlopeFactor = 0.0f;

		// multisampling seems very useful, but disable for now
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // optional
		multisampling.pSampleMask = nullptr; // optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // optional
		multisampling.alphaToOneEnable = VK_FALSE; // optional

		// no depthstencilstate createinfo struct, we'll just pass a nullptr

		// color blending
		// two structs, the first is per framebuffer
		// the second is global color blending settings
		VkPipelineColorBlendAttachmentState colorBlendAttachment{}; 
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT
			| VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT
			| VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // optional
		// the ops do somthing like
		/*
		if(blendEnable) {
			finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> 
				(dstColorBlendFactor * oldColor.rgb);
			// same for alpha
		} else {
			finalColor = newColor;
		}
		finalColor = finalColor & colorWriteMask;
		*/

		// second blending structure
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE; 
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // optional
		colorBlending.attachmentCount = 1; // one attachment for one framebuffer
		colorBlending.pAttachments = &colorBlendAttachment; // the attachement
		colorBlending.blendConstants[0] = 0.0f; //optional
		colorBlending.blendConstants[1] = 0.0f; //optional
		colorBlending.blendConstants[2] = 0.0f; //optional
		colorBlending.blendConstants[3] = 0.0f; //optional
		// logic ops being set to true would disable all alpha blending for all framebuffers, colorWriteMask still used
		// we've disabled both modes so that colors just go through unmodified

		// if you want to allow dynamic state, ten you would have to create a VkDynamicState struct, 
		// which would cause the values to be ignored.
		// I'll use a nullptr right now.
		// things that can be made dynamic: Viewport, line_width, blend_constants


		// Pipeline layout (uniform setup)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 0; // optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // optional

		if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
			throw std::runtime_error("failed to create pipeline layout!");
		}

		// create the pipeline

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		// shader stages
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		// struct pointers
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // optional
		
		// layout handle
		pipelineInfo.layout = pipelineLayout;
		// render pass
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0; // index of subpass

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // optional
		// vulkan allows setting up a new pipeline 
		// by deriving from an existing pipeline, which can be very handy and be more efficient
		pipelineInfo.basePipelineIndex = -1; // optional
		// index is an alternative way to specify a parent if you are about to create the parent pipeline
		// these are only used if VK_PIPELINE_CREATE_DERIVATIVE_BIT flag is set
		// in the flags field

		// note that you can create multiple piplines in one call
		// the second arg:
		// pipeline cache is used to store and reuse data relevant to pipeline creation across multiple  
		// calls to vkCreateGraphicsPipelines (and even across executions if stored to a file)
		// later chapter on this in tutorial
		if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create graphics pipeline!"); 
		}

		// destroy the shader modules 
		//vkDestroyShaderModule(device, fragShaderModule, nullptr);
		//vkDestroyShaderModule(device, vertShaderModule, nullptr);
	}

	VkShaderModule createShaderModule(const std::vector<char>& code) {
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // needs to satisfy the alignment 
		// requirements, but vectors already ensure that

		VkShaderModule module;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &module) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}
		return module;
	}

	void createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); ++i) {
			VkImageView attachments[] = {
				swapChainImageViews[i]
			};

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass; // render pass needs to be compatible with
			framebufferInfo.attachmentCount = 1; // image views that should be bound to the attachment descriptons
			// in the renderPass pAttachment array
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to create framebuffer!");
			}
		}
	}

	void createCommandPool() {
		QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		poolInfo.flags = 0; // optional
		// flags are TRANSIENT: command buffers are rerecorded often
		// RESET_COMMAND_BUFFER: allow buffers to be rerecorded individually, otherwise they 
		// all have to be reset together
		// we are just recording the command buffers once and not rerecording, so we don't need either flag
		if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}
	}

	void createVertexBuffer() {
		/*
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = sizeof(vertices[0]) * vertices.size();

		bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // don't need to share across queues
		// leave flags parameter at 0 for now

		if (vkCreateBuffer(device, &bufferInfo, nullptr, &vertexBuffer) != VK_SUCCESS) {
			throw std::runtime_error("failed to create vertex buffer!");
		}

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(device, vertexBuffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (vkAllocateMemory(device, &allocInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate vertex buffer memory!");
		}

		vkBindBufferMemory(device, vertexBuffer, vertexBufferMemory, 0);
		// fourth parameter is offset
		// if nonzero, offset must be divisible by memReq.alignment

		*/

		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		JBuffer stagingBuffer(
			physicalDevice,
			device,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, // used as source to transfer data to another buffer
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT // mappable
		); // automatically destroyed when it goes out of scope

		void* data;
		// params are device, memory, offset, size, flags, ptr to ptr
		vkMapMemory(device, stagingBuffer.memory(), 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), (size_t)bufferSize);
		vkUnmapMemory(device, stagingBuffer.memory());
		// driver may not immediately copy the data on write,
		// two strategies:
		// VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		// or call
		// flushmappedmemory ranges after writing to mapped
		// invalidate mapped memory ranges before reading


		vertBuffer = new JBuffer(
			physicalDevice,
			device,
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		copyBuffer(stagingBuffer.buffer(), vertBuffer->buffer(), bufferSize);

		// staging buffer goes out of scope and gets cleaned up
		// NOTE: IRL should not allocate memory for every buffer, 
		// should use a custom allocator, or use the VulkanMemoryAllocator library
		// however, for this tutorial, will be ok.
	}

	void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
		// need to allocate a temporary command buffer
		// might want a separate command pool for temporary command buffers so that 
		// we can pass the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag
		// the implementation might be able to optimize in that case
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary command buffer
		allocInfo.commandPool = commandPool; // main command pool
		allocInfo.commandBufferCount = 1; // one command buffer

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);
		
		// record it
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // only going to use this once

		vkBeginCommandBuffer(commandBuffer, &beginInfo);
		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // optional
		copyRegion.dstOffset = 0; // optional
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion); // takes an array of regions
		// note not allowed to specify VK_WHOLE_SIZE for copyRegion.size
		vkEndCommandBuffer(commandBuffer); // just want to copy, so stop recording

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(graphicsQueue); // could also use a fence and wait on the fence

		vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
	}

	// leaving this here. It's been sort of moved to JBuffer
	uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i) {
			if ((typeFilter & (1 << i)) // check if the ith bit in type filter is 1
				&& (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				// check to make sure it has all of the properties
				return i;
			}
		}

	}

	void createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());
		
		// need allocateInfo
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary or secondary
		// primary can be submitted to a queue for execution, but not called from other buffers
		// secondary can be called from other buffers, but not submitted directly
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
			throw std::runtime_error("failed to allocate command buffers!");
		}

		for (size_t i = 0; i < commandBuffers.size(); ++i) {
			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = 0; // optional
			// flags: ONE_TIME_SUBMIT (rerecorded after executing once)
			// RENDER_PASS_CONTINUE (secondary command buffer entirely w/in a single render pass)
			// SIMULTANEOUS_USE (can be resubmitted while it is already pending execution)
			beginInfo.pInheritanceInfo = nullptr; // optional
			// only for secondary command buffers, what state to inherit from primary command buffers

			// beginCommandBuffer will reset the command buffer (implicitly)
			if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
				throw std::runtime_error("failed to begin recording command buffer!");
			}

			VkRenderPassBeginInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			// framebuffer to render into

			renderPassInfo.renderArea.offset = { 0,0 };
			renderPassInfo.renderArea.extent = swapChainExtent; // render to full area of image. 
			// pixels outside this area have undefined values

			VkClearValue  clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
			renderPassInfo.clearValueCount = 1;
			renderPassInfo.pClearValues = &clearColor; // not sure why this is an array, perhaps if we had
			// several layers or something?
			// clear color to use for LOAD_OP_CLEAR 

			vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			// start the render pass, vkCmd prefix identifies functions that record commands
			// SUBPASS_CONTENTS: INLINE (render pass commands are in primary command buffer, no secondary command buffers)
			// SECONDARY_COMMAND_BUFFERS (render pass commands will be executed from secondary command buffers)
			
			// bind the pipeline
			vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			
			VkBuffer vertexBuffers[] = { vertBuffer->buffer() };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);

			// draw has parameters
			// vertexCount (3 vertices)
			// instanceCount (for instanced rendering, 1 if not doing instanced rendering)
			// firstVertex (offset into vertex buffer, defines lowest value of gl_VertexIndex)
			// firstInstance (used as an offset for instanced rendering, lowest value of gl_InstanceIndex)
			vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(vertices.size()), 1, 0, 0);
			vkCmdEndRenderPass(commandBuffers[i]);
			if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
				throw std::runtime_error("failed to record command buffer!");
			}
		}
	}

	void createSyncObjects() {
		// semaphores for GPU-GPU sync
		// fences for CPU-GPU sync
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT); 
		imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE); // initialize to no fence to start with

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		// no other required fields right now

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // create fences signaled for first frames

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS
				|| vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS
				|| vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
				) {
				throw std::runtime_error("failed to create semaphores for a frame!");
			}
		}
	}

	void mainLoop() {
		// check for events until the window should close
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			drawFrame();
		}

		vkDeviceWaitIdle(device);
	}

	void drawFrame() {
		// wait for fence for current frame
		// waits on an array of fences, true means wait for all of them
		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		
		uint32_t imageIndex;
		// params: 
		// logical device, swapchain we want image from
		// timeout in nanoseconds, UINT64_MAX disables timeout
		// synchronization objects Semaphore, Fence, or both, once signaled, we can draw
		// last one is index of image that will be available
		VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], 
			VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR) // swap chain out of date, recreate
		{
			recreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { // suboptimal means swap chain 
			// not great, but will still work (it is a "success" code)
			throw std::runtime_error("failed to acquire swap chain image"); 
		}

		// wait until this image is free, (i.e., not being used by a previous frame)
		if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
			vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
		}
		// mark image as being in use by a frame
		imagesInFlight[imageIndex] = inFlightFences[currentFrame];

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		// which semaphores to wait on, and in which stages of the pipeline to wait
		// we want to wait on writing colors until image is available.
		// wait stages <-> wait semaphores
		
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];

		VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores; // which semaphores to signal once 
		// command buffers have finished execution

		vkResetFences(device, 1, &inFlightFences[currentFrame]); // reset the fence for this frame

		// Queue submit takes an array of submit info structures 
		// last parameter is an optional fence signaled when the command buffers finish execution
		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
			throw std::runtime_error("failed to submit draw command buffer!");
		}

		// Now just need to present the frame to screen!

		VkPresentInfoKHR presentInfo{}; // info to configure the presentation
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores; // render finished semaphore
		// the semaphore to wait on
		VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex; 
		// params specify swap chains to present images to and the index of the image for each swap chain
		// almost always a single one
		presentInfo.pResults = nullptr; // optional
		// array of VkResult values to check if each presentation was successful
		// only necessary if you have more than one swap chain, since we can just use the return value 
		// of:
		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		// error handling to come later

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
			framebufferResized = false; // need to resize after vkQueuePresentKHR to ensure
			// that semaphores are in a consistent state. Otherwise a signalled semaphore may never
			// be properly waited upon
			recreateSwapChain(); // recreate if suboptimal this time, since we've already presented the frame
		}
		else if (result != VK_SUCCESS) {
			throw std::runtime_error("failed to present swap chain image!");
		}

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	// note that command pools only depend on the logical device, not the swap chain.
	void createSwapChainAndFollowing() {
		createSwapChain();
		createImageViews();
		createRenderPass();
		createGraphicsPipeline();
		createFramebuffers();
		createCommandBuffers();
	}

	void recreateSwapChain() {
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		while (width == 0 || height == 0) {
			// minimized
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}

		vkDeviceWaitIdle(device);

		cleanupSwapChain();
		createSwapChainAndFollowing();
	}

	void cleanupSwapChain() {

		for (auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(device, framebuffer, nullptr);
		}

		// frees command buffers without recreating the command pool
		vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());

		vkDestroyPipeline(device, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);

		for (auto imageView : swapChainImageViews) {
			vkDestroyImageView(device, imageView, nullptr);
		}

		vkDestroySwapchainKHR(device, swapChain, nullptr);
	}

	void cleanup() {
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}

		cleanupSwapChain();

		delete vertBuffer;
		//vkDestroyBuffer(device, vertexBuffer, nullptr);
		//vkFreeMemory(device, vertexBufferMemory, nullptr); // can be freed when the buffer is not longer in use

		vkDestroyCommandPool(device, commandPool, nullptr);

		vkDestroyDevice(device, nullptr); // destroy the logical device

		vkDestroySurfaceKHR(instance, surface, nullptr); // destroy the surface

		if (enableValidationLayers) {
			// destroy debug callback
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}

		vkDestroyInstance(instance, nullptr);

		glfwDestroyWindow(window);

		glfwTerminate();
	}

	// debug callback for the DebugUtils extension
	// message severity is one of Verbose, Info, Warning, Error, (plus some?) values ordered by increasing severity
	// message type is one of General (unrelated to spec/performance), Validation (violates spec), Performance (nonoptimal use)
	// callback data is a pointer to a callback data struct containing pMessage (message string), 
	// pObjects (array of related objs), objectCount (length of pObjects)
	// pUserData is a pointer specified during setup of the callback
	// return value: True means abort the call with VK_ERROR_VALIDATION_FAILED_EXT error
	// this should not be used usually, so always return VK_FALSE.
	static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData) {
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

		return VK_FALSE;
	}

};


int main() {
	HelloTriangleApplication app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}