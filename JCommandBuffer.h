#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <stdexcept>

#include "JCommandPool.h"


class JCommandBuffers;

// view of a single VkCommandBuffer in a JCommandBuffers
// has no ownership
class JCommandBuffer
{
	friend class JCommandBuffers;
protected:
	const JCommandBuffers* _buffers = nullptr;

	VkCommandBuffer _buff = VK_NULL_HANDLE;

	JCommandBuffer(const JCommandBuffers* buffers, VkCommandBuffer buff);

public:
	inline VkCommandBuffer buffer() const { return _buff; }

	inline int beginCommandBuffer(VkCommandBufferBeginInfo* info) { return vkBeginCommandBuffer(_buff, info); }
	inline int endCommandBuffer() { return vkEndCommandBuffer(_buff); }
};

// acquires and manages command buffers from a command pool
class JCommandBuffers
{
protected:
	std::vector<VkCommandBuffer> _buffers;

	VkCommandBufferLevel _level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	const JCommandPool* _pool = nullptr;

public:
	JCommandBuffers() = delete;
	JCommandBuffers(const JCommandBuffers&) = delete;
	void operator=(const JCommandBuffers&) = delete;

	JCommandBuffers(const JCommandPool* pool, uint32_t N = 1, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
	virtual ~JCommandBuffers();

	inline JCommandBuffer operator[](uint32_t index) const {
		if (index < 0 || index >= _buffers.size()) {
			throw std::runtime_error("index out of range for JCommandBuffers");
		}
		return JCommandBuffer(this, _buffers[index]);
	}

	inline uint32_t size() { return _buffers.size(); }

	template<typename F, typename T>
	static T withTransientCommandBuffer(const JCommandPool* pool, F function, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
		JCommandBuffers b(pool, 1, level);
		return function(b[0]);
		// b is automatically destroyed here
	}
};




