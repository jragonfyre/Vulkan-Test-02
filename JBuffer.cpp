#include "JBuffer.h"

uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties) {
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

JBuffer::JBuffer(VkPhysicalDevice physicalDevice,
	VkDevice device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties)
	: _physDevice(physicalDevice)
	, _device(device)
	, _size(size)
	, _usage(usage)
	, _properties(properties)
{
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = _size;

	bufferInfo.usage = _usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // don't need to share across queues
	// for now
	// leave flags parameter at 0 for now

	if (vkCreateBuffer(_device, &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_device, _buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(_physDevice, memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(device, &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, _buffer, _bufferMemory, 0);
}

JBuffer::~JBuffer()
{
	vkDestroyBuffer(_device, _buffer, nullptr);
	vkFreeMemory(_device, _bufferMemory, nullptr);
}
