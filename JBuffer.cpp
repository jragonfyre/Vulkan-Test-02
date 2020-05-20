#include "JBuffer.h"
#include "vkutils.h"
#include <stdexcept>



JBuffer::JBuffer(
	//VkPhysicalDevice physicalDevice,
	//VkDevice device,
	const JDevice* device,
	VkDeviceSize size,
	VkBufferUsageFlags usage,
	VkMemoryPropertyFlags properties)
	: _pDevice(device)
	//_physDevice(physicalDevice)
	//, _device(device)
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

	if (vkCreateBuffer(_pDevice->device(), &bufferInfo, nullptr, &_buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_pDevice->device(), _buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(_pDevice->physical(), memRequirements.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	if (vkAllocateMemory(_pDevice->device(), &allocInfo, nullptr, &_bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(_pDevice->device(), _buffer, _bufferMemory, 0);
}

JBuffer::~JBuffer()
{
	vkDestroyBuffer(_pDevice->device(), _buffer, nullptr);
	vkFreeMemory(_pDevice->device(), _bufferMemory, nullptr);
}
