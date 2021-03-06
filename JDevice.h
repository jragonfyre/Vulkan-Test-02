#pragma once

#include <vulkan/vulkan.h>
#include "vkutils.h"

#include <vector>
#include <string>


enum class JQueueType {
	JGraphicsQueue, JPresentQueue
};

class JDevice
{
protected:
	VkPhysicalDevice _physical = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;

	QueueFamilyIndices _indices{};

	VkDevice _device = VK_NULL_HANDLE;

	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;

	//bool reference = false;

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
	inline const QueueFamilyIndices& queueIndices() const { return _indices; }

	inline VkQueue getQueue(JQueueType type) const {
		switch (type) {
		case JQueueType::JGraphicsQueue:
			return _graphicsQueue;
		case JQueueType::JPresentQueue:
			return _presentQueue;
		}
	}
private:
	//void nullify();

};



