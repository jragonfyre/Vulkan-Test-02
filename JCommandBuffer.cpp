#include "JCommandBuffer.h"
#include <stdexcept>

JCommandBuffers::JCommandBuffers(const JCommandPool* pool, uint32_t N, VkCommandBufferLevel level)
	: _pool(pool)
	, _level(level)
{

	_buffers.resize(N);

	// need allocateInfo
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = nullptr; //_pool->pool(); should be set by _pool when you call allocateCommandBuffers
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // primary or secondary
	// primary can be submitted to a queue for execution, but not called from other buffers
	// secondary can be called from other buffers, but not submitted directly
	allocInfo.commandBufferCount = (uint32_t)_buffers.size();

	if (_pool->allocateCommandBuffers(allocInfo, _buffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

JCommandBuffers::~JCommandBuffers()
{
	_pool->freeCommandBuffers(static_cast<uint32_t>(_buffers.size()), _buffers.data());
}

JCommandBuffer::JCommandBuffer(const JCommandBuffers* buffers, VkCommandBuffer buff)
	: _buffers(buffers)
	, _buff(buff)
{
}
