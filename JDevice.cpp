#include "JDevice.h"
#include <set>
#include <stdexcept>


JDevice::JDevice(VkPhysicalDevice physical, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, bool enableValidationLayers, const std::vector<const char*>& validationLayers)
	: _physical(physical)
	, _surface(surface)
	, _indices{}
	, _device(VK_NULL_HANDLE)
	, _graphicsQueue(VK_NULL_HANDLE)
	, _presentQueue(VK_NULL_HANDLE)
	, _deviceExtensions(&deviceExtensions)
{
	_indices = findQueueFamilies(_physical, _surface);

	// vector of queue infos
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	// set of distinct queue indices
	std::set<uint32_t> uniqueQueueFamilies = { _indices.graphicsFamily.value(), _indices.presentFamily.value() };

	float queuePriority = 1.0f;
	for (uint32_t queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;//_indices.graphicsFamily.value();
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

	createInfo.enabledExtensionCount = static_cast<uint32_t>(_deviceExtensions->size());
	createInfo.ppEnabledExtensionNames = _deviceExtensions->data();


	// not needed for modern vulkan, but for compatibility, include validation layers on device createinfo
	if (enableValidationLayers) {
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	// create the logical device
	if (vkCreateDevice(_physical, &createInfo, nullptr, &_device) != VK_SUCCESS) {
		throw std::runtime_error("failed to create logical device!");
	}

	// finally retrieve the queue handles (the queues are created with the device)
	// 0 is the index of the queue in the family, since we're only creating one.
	vkGetDeviceQueue(_device, _indices.graphicsFamily.value(), 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, _indices.presentFamily.value(), 0, &_presentQueue);
}

JDevice::~JDevice()
{
	vkDestroyDevice(_device, nullptr);
}
