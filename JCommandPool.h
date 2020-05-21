#pragma once

#include "JDevice.h"



class JCommandPool
{
protected:
	VkCommandPool _pool;
	const JDevice* _device;
	JQueueType _type;
	VkCommandPoolCreateFlags _flags;

public:
	JCommandPool() = delete;
	JCommandPool(const JCommandPool&) = delete;
	void operator=(const JCommandPool&) = delete;

	JCommandPool(const JDevice* device, VkCommandPoolCreateFlags flags = 0, JQueueType type = JQueueType::JGraphicsQueue);
	~JCommandPool();

	inline VkCommandPool pool() const { return _pool; }
	inline JQueueType type() const { return _type; }
	inline VkCommandPoolCreateFlags creationFlags() const { return _flags; }
	
	// custom methods
	inline VkQueue queue() const { return _device->getQueue(_type); }


	// vulkan proxies

	// fills in the pool information
	// might change this later
	inline int allocateCommandBuffers(VkCommandBufferAllocateInfo& allocInfo, VkCommandBuffer* buffers) const { 
		allocInfo.commandPool = _pool;
		return vkAllocateCommandBuffers(_device->device(), &allocInfo, buffers); 
	}
	inline void freeCommandBuffers(uint32_t N, VkCommandBuffer* buffers) const {
		vkFreeCommandBuffers(_device->device(), _pool, N, buffers);
	}
};