#pragma once

#include <vulkan/vulkan.h>
#include "vkutils.h"

#include <vector>
#include <string>

class JDevice
{
protected:
	VkPhysicalDevice _physical;
	VkSurfaceKHR _surface;

	QueueFamilyIndices _indices;

	VkDevice _device;

	VkQueue _graphicsQueue;
	VkQueue _presentQueue;

	const std::vector<const char*>* _deviceExtensions;

public:
	JDevice() = delete;
	JDevice(const JDevice&) = delete;
	void operator=(const JDevice&) = delete;

	JDevice(VkPhysicalDevice physical, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, bool enableValidationLayers, const std::vector<const char*>& validationLayers);
	virtual ~JDevice();

	inline VkDevice device() const { return _device; }
	inline VkPhysicalDevice physical() const { return _physical; }
	inline VkQueue graphicsQueue() const { return _graphicsQueue; }
	inline VkQueue presentQueue() const { return _presentQueue; }
};

