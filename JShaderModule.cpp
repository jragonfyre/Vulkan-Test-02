#include "JShaderModule.h"

#include <stdexcept>
#include "utils.h"




//JShaderModule::JShaderModule(VkDevice device, JShaderType type, const std::vector<char>& code, const char* entrypoint) : device(device), _type(type), _entrypoint(entrypoint) {
JShaderModule::JShaderModule(const JDevice* device, JShaderType type, const std::vector<char>& code, const char* entrypoint) : _pDevice(device), _type(type), _entrypoint(entrypoint) {
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data()); // needs to satisfy the alignment 
	// requirements, but vectors already ensure that
	
	if (vkCreateShaderModule(_pDevice->device(), &createInfo, nullptr, &_module) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}
}


JShaderModule::JShaderModule(const JDevice* device, JShaderType type, const std::string fname, const char* entrypoint) : JShaderModule(device, type, readFile(fname), entrypoint) {

}


JShaderModule::JShaderModule(const JDevice* device, JShaderType type, const char* fname, const char* entrypoint) : JShaderModule(device, type, std::string(fname), entrypoint)
{
}

JShaderModule::~JShaderModule() {
	vkDestroyShaderModule(_pDevice->device(), _module, nullptr);
}

VkPipelineShaderStageCreateInfo JShaderModule::stageInfo() const
{
	VkPipelineShaderStageCreateInfo info{};

	info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	info.stage = flagBits(_type); // vertex shader stage
	info.module = module(); //vertShaderModule;
	info.pName = _entrypoint; // entry point
	// not usng pSpecializationInfo, but you can pass in compile time constants, and compiler can optimize 
	// with these values, we're setting it to nullptr
	return info;
}

VkShaderStageFlagBits flagBits(JShaderType type)
{
	VkShaderStageFlagBits flags{};
	if (type & JShaderType::JVertex) {
		flags = static_cast<VkShaderStageFlagBits>(flags | VK_SHADER_STAGE_VERTEX_BIT);// | VK_SHADER_STAGE_VERTEX_BIT;
	}
	if (type & JShaderType::JFragment) {
		flags = static_cast<VkShaderStageFlagBits>(flags | VK_SHADER_STAGE_FRAGMENT_BIT);
	}
	return flags;
}
