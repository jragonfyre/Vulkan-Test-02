#pragma once

#include <vulkan/vulkan.h>
#include <string>
#include "JDevice.h"

class JImage
{
protected:
	VkImage _image;
	VkDeviceMemory _memory;
	uint32_t _width, _height;
	
	//VkPhysicalDevice _physical;
	//VkDevice _device;
	const JDevice* _pDevice;

	VkFormat _format;
	VkImageTiling _tiling;
	VkImageUsageFlags _usage;
	VkMemoryPropertyFlags _properties;

	std::string _filename;

public:
	JImage() = delete;
	JImage(const JImage&) = delete;
	void operator=(const JImage&) = delete;

	JImage(
		//VkPhysicalDevice physical, 
		//VkDevice device, 
		const JDevice* device,
		std::string fname,
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	JImage(
		//VkPhysicalDevice physical,
		//VkDevice device,
		const JDevice* device,
		uint32_t width,
		uint32_t height,
		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB,
		VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL,
		VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	virtual ~JImage();

	inline VkImage image() const { return _image; }
	inline VkDeviceMemory memory() const { return _memory; }

	inline uint32_t width() const { return _width; }
	inline uint32_t height() const { return _height; }

	inline VkExtent2D extent() const { VkExtent2D ans{}; ans.height = height(); ans.width = width(); return ans; }

private:
	void initializeImage();
};

