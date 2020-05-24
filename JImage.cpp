#include "JImage.h"

#include <stb_image.h>
#include <stdexcept>
#include "JBuffer.h"
#include "vkutils.h"
#include "JCommandBuffer.h"



JImage::JImage(
	//VkPhysicalDevice physical,
	//VkDevice device,
	const JDevice* device,
	const JCommandPool* pool,
	std::string fname,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties)
	: _pDevice(device)
	, _pool(pool)
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

	transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	copyBufferToImage(&stagingBuffer);

	transitionImageLayout(VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	
	// todo: consider moving image loading code elsewhere
}

JImage::JImage(
	//VkPhysicalDevice physical,
	//VkDevice device,
	const JDevice* device,
	const JCommandPool* pool,
	uint32_t width,
	uint32_t height,
	VkFormat format,
	VkImageTiling tiling,
	VkImageUsageFlags usage,
	VkMemoryPropertyFlags properties)
	: _pDevice(device)
	, _pool(pool)
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


// could modify to take an argument for making oldLayout= VK_IMAGE_LAYOUT_UNDEFINED, when we don't care about 
// the existing contents
void JImage::transitionImageLayout(VkFormat format, VkImageLayout newLayout)
{
	JCommandBuffers::runWithSingleTimeCommandBuffer(_pool, [this, newLayout](JCommandBuffer buffer) {
		VkImageLayout oldLayout = this->currentLayout();
		
		VkImageMemoryBarrier barrier{}; // used to synchronize access to resoures, equivalent buffer memory barrier
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // not using barrier to transfer queue family ownership
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; // not using barrier to transfer queue family ownership
		// note these are not the default values!
		barrier.image = this->image();
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		// TODO the following two lines
		barrier.srcAccessMask = 0; // which ops must happen before barrier
		barrier.dstAccessMask = 0; // which ops must happen after barrier
		// we'll come back to this later

		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		// compatibility table:
		// https://www.khronos.org/registry/vulkan/specs/1.0/html/vkspec.html#synchronization-access-types-supported

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0; // not waiting on anything
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT; 

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // at the beginning, nothing needs to happen before write
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // required
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		}

		vkCmdPipelineBarrier(
			buffer.buffer(),
			0 /*TODO*/, 0 /*TODO*/, // pipeline stage in which ops occcur that happen before the barrier
			// then pipeline stage in which ops will wait on the barrier
			0, // 0 or VK_DEPENDENCY_BY_REGION_BIT, latter is a by region condition, meaning implementation
			// can begin reading from the parts that were already written so far
			0, nullptr, // arrays of memory barriers
			0, nullptr, // arrays of buffer memory barriers
			1, &barrier // arrays of image memory barriers
			);
		});
	_layout = newLayout; 
}

void JImage::copyBufferToImage(const JBuffer* buffer)
{
	JCommandBuffers::runWithSingleTimeCommandBuffer(_pool, [this, buffer](JCommandBuffer cmdBuffer) {
		VkBufferImageCopy region{};
		region.bufferOffset = 0; // where pixels start
		region.bufferRowLength = 0; // could have padding bytes between rows in image, 0 means tightly packed
		region.bufferImageHeight = 0; // same for height

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0,0,0 };
		region.imageExtent = {
			this->width,
			this->height,
			1
		};

		vkCmdCopyBufferToImage(
			cmdBuffer.buffer(),
			buffer->buffer(),
			this->image(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // layout the image is currently in, assume it's in good 
			// transfer state
			1, // region count
			&region // region array
		);
		});
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
