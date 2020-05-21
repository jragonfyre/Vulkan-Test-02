#include "JCommandPool.h"

#include <stdexcept>


JCommandPool::JCommandPool(const JDevice* device, VkCommandPoolCreateFlags flags, JQueueType type)
	: _device(device)
	, _flags(flags)
	, _type(type)
{
	const QueueFamilyIndices& queueFamilyIndices = device->queueIndices();

	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	switch (type) {
	case JQueueType::JGraphicsQueue:
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
		break;
	case JQueueType::JPresentQueue:
		poolInfo.queueFamilyIndex = queueFamilyIndices.presentFamily.value();
		break;
	default:
		throw std::runtime_error("unrecognized queue type for creating command pool");
	}
	poolInfo.flags = _flags;
	// flags are TRANSIENT: command buffers are rerecorded often
	// RESET_COMMAND_BUFFER: allow buffers to be rerecorded individually, otherwise they 
	// all have to be reset together
	// we are just recording the command buffers once and not rerecording, so we don't need either flag
	if (vkCreateCommandPool(device->device(), &poolInfo, nullptr, &_pool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}
}

JCommandPool::~JCommandPool()
{
	vkDestroyCommandPool(_device->device(), _pool, nullptr);
}
