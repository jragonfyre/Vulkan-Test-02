#include "JImage.h"

#include <stb_image.h>
#include <stdexcept>
#include "JBuffer.h"
#include "vkutils.h"



JImage::JImage(
	//VkPhysicalDevice physical,
	//VkDevice device,
	const JDevice* device,
	std::string fname,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties)
	: _pDevice(device)
	// _physical(physical)
	//, _device(device)
	, _image(VK_NULL_HANDLE)
	, _memory(VK_NULL_HANDLE)
	, _format(format)
	, _tiling(tiling)
	, _usage(usage)
	, _properties(properties)
	, _filename(fname)
{
	// "textures/stones-1000x1000.jpg"

	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(fname.c_str(), &texWidth, &texHeight, &texChannels,
		STBI_rgb_alpha); // the STBI_rgb_alpha forces loading with an alpha channel
	VkDeviceSize imageSize = (uint64_t)texWidth * texHeight * 4;

	if (!pixels) {
		throw std::runtime_error("failed to load texture image!" + fname);
	}

	_width = static_cast<uint32_t>(texWidth);
	_height = static_cast<uint32_t>(texHeight);

	JBuffer stagingBuffer(
		//_physical,
		_pDevice,
		imageSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

	void* data;
	vkMapMemory(_pDevice->device(), stagingBuffer.memory(), 0, imageSize, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(imageSize));
	vkUnmapMemory(device->device(), stagingBuffer.memory());

	stbi_image_free(pixels); // clean up pixel array

	initializeImage();

	// TODO copy memory over
}

JImage::JImage(
	//VkPhysicalDevice physical,
	//VkDevice device,
	const JDevice* device,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties)
	: _pDevice(device)
	// _physical(physical)
	//, _device(device)
	, _image(VK_NULL_HANDLE)
	, _memory(VK_NULL_HANDLE)
	, _format(format)
	, _tiling(tiling)
	, _usage(usage)
	, _properties(properties)
	, _width(width)
	, _height(height)
	, _filename("unnamed image")
{
	initializeImage();
}

JImage::~JImage()
{
	vkDestroyImage(_pDevice->device(), _image, nullptr);
	vkFreeMemory(_pDevice->device(), _memory, nullptr);
}

void JImage::initializeImage()
{
	VkImageCreateInfo imageInfo{};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = _width;
	imageInfo.extent.height = _height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = _format; // needs to be same format for texels as pixels in buffer
	// otherwise copy op will fail
	imageInfo.tiling = _tiling; // either linear or optimal. Linear means texels are 
	// laid out in row-major order
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // other option is PREINITIALIZED
	// images can be transitioned, UNDEFINED means 1st transition will discard texels, PREINITIALIZED means preserve
	// them. Basically PREINITIALIZED is only useful if you want to upload texel data and then 
	// transition to transfer source (if we were using a staging image instead of a staging buffer for example)
	imageInfo.usage = _usage;
	// fairly self explanator, we want to be a transfer dest, and we want to be able to sample the image
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	// only used by one queue family, graphics (which supports transfer ops)
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // related to multisampling
	// only relevant for images used as attachments, which ours is not
	imageInfo.flags = 0; //optional
	// flags is related to sparse images, where only certain regions are actually in memory
	// useful for 3d textures for example.

	if (vkCreateImage(_pDevice->device(), &imageInfo, nullptr, &_image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
		// in principle, might need to check for support for the image format, but r8g8b8a8 is super common
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_pDevice->device(), _image, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = findMemoryType(
		_pDevice->physical(),
		//_physical,
		memRequirements.memoryTypeBits,
		_properties);

	if (vkAllocateMemory(_pDevice->device(), &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory! " + _filename);
	}

	vkBindImageMemory(_pDevice->device(), _image, _memory, 0);
}
