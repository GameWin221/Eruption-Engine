#include "Pipeline.hpp"

#include <Renderer/Context.hpp>

namespace en
{
	Pipeline::~Pipeline()
	{
		UseContext();

		if (m_Layout != VK_NULL_HANDLE)
			vkDestroyPipelineLayout(ctx.m_LogicalDevice, m_Layout, nullptr);

		if (m_Pipeline != VK_NULL_HANDLE)
			vkDestroyPipeline(ctx.m_LogicalDevice, m_Pipeline, nullptr);
	}

	void Pipeline::PushConstants(const void* data, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage)
	{
		vkCmdPushConstants(m_BoundCommandBuffer, m_Layout, shaderStage, offset, size, data);
	}
	void Pipeline::BindDescriptorSet(Handle<DescriptorSet> descriptor, uint32_t index, VkPipelineBindPoint bindPoint)
	{
		const VkDescriptorSet descriptorSet = descriptor->GetHandle();
		vkCmdBindDescriptorSets(m_BoundCommandBuffer, bindPoint, m_Layout, index, 1U, &descriptorSet, 0U, nullptr);
	}
	void Pipeline::BindDescriptorSet(VkDescriptorSet descriptor, uint32_t index, VkPipelineBindPoint bindPoint)
	{
		vkCmdBindDescriptorSets(m_BoundCommandBuffer, bindPoint, m_Layout, index, 1U, &descriptor, 0U, nullptr);
	}

	VkPipelineShaderStageCreateInfo Pipeline::CreateShaderInfo(VkShaderModule shaderModule, VkShaderStageFlagBits stage)
	{
		return VkPipelineShaderStageCreateInfo {
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = stage,
			.module = shaderModule,
			.pName = "main"
		};
	}

	VkShaderModule Pipeline::CreateShaderModule(const std::string& path)
	{
		VkShaderModule shaderModule{};

		std::vector<char> shaderCode = ReadShaderFile(path);

		VkShaderModuleCreateInfo createInfo {
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shaderCode.size(),
			.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data())
		};

		if (vkCreateShaderModule(Context::Get().m_LogicalDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
			EN_ERROR("Shader::Shader() - Failed to create shader module!");

		return shaderModule;
	}

	void Pipeline::DestroyShaderModule(VkShaderModule shaderModule)
	{
		if (shaderModule != VK_NULL_HANDLE)
			vkDestroyShaderModule(Context::Get().m_LogicalDevice, shaderModule, nullptr);
	}

	std::vector<char> Pipeline::ReadShaderFile(const std::string& path)
	{
		std::ifstream file(path, std::ios::ate | std::ios::binary);

		if (!file.is_open())
			EN_ERROR("Pipeline::ReadShaderFile() - Failed to open shader source file!");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		if (buffer.empty())
			EN_WARN("Pipeline::ReadShaderFile() - The read buffer is empty!");

		return buffer;
	}

}