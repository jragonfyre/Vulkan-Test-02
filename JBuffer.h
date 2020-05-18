#pragma once

#include <vulkan/vulkan.h>

#include <stdexcept>


uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);

class JBuffer
{
protected:
	VkBuffer _buffer;
	VkDeviceMemory _bufferMemory;
	VkDevice _device;
	VkPhysicalDevice _physDevice;
	VkDeviceSize _size;
	VkBufferUsageFlags _usage;
	VkMemoryPropertyFlags _properties;

public:

	inline VkBuffer buffer() { return _buffer; }
	inline VkDeviceMemory memory() { return _bufferMemory; }
	
	JBuffer(
		VkPhysicalDevice physicalDevice,
		VkDevice device,
		VkDeviceSize size,
		VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties);

	JBuffer() = delete;
	JBuffer(const JBuffer& other) = delete;

	~JBuffer();
};

