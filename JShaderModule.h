#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include "JDevice.h"

enum JShaderType : uint32_t {
	JVertex = 0x1, 
	JFragment = 0x2
};

VkShaderStageFlagBits flagBits(JShaderType type);

class JShaderModule
{
protected:
	VkShaderModule _module;
	const JDevice* _pDevice;
	//VkDevice _device;
	JShaderType _type;
	const char* _entrypoint;
public:
	//JShaderModule(const JDevice* device, JShaderType type, const char* fname, const char* entrypoint = "main");
	//JShaderModule(VkDevice device, JShaderType type, const char* fname, const char* entrypoint = "main");
	//JShaderModule(VkDevice device, JShaderType type, const std::vector<char>& code, const char* entrypoint = "main");
	//JShaderModule(VkDevice device, JShaderType type, const std::string fname, const char* entrypoint = "main");
	JShaderModule(const JDevice* device, JShaderType type, const char* fname, const char* entrypoint = "main");
	JShaderModule(const JDevice* device, JShaderType type, const std::vector<char>& code, const char* entrypoint = "main");
	JShaderModule(const JDevice* device, JShaderType type, const std::string fname, const char* entrypoint = "main");

	JShaderModule() = delete;
	JShaderModule(const JShaderModule& other) = delete;
	virtual ~JShaderModule();
	inline VkShaderModule module() const { return _module; }
	VkPipelineShaderStageCreateInfo stageInfo() const;
};

