#pragma once

#include <vulkan/vulkan.h>

#include "JDevice.h"



class JBuffer
{
protected:
	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;
	//VkDevice _device;
	//VkPhysicalDevice _physDevice;
	const JDevice* _pDevice;
	VkDeviceSize _size;
	VkBufferUsageFlags _usage;
	VkMemoryPropertyFlags _properties;

public:

	inline VkBuffer buffer() { return _buffer; }
	inline VkDeviceMemory memory() { return _bufferMemory; }
	
	JBuffer(
		//VkPhysicalDevice physicalDevice,
		//VkDevice device,
		const JDevice* device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

	JBuffer() = delete;
	JBuffer(const JBuffer&) = delete;
	void operator=(const JBuffer&) = delete;

	virtual ~JBuffer();
};

