#include "Core/EnPch.hpp"
#include "ComputeShader.hpp"

namespace en
{
	ComputeShader::ComputeShader(const ComputeInfo& createInfo) : m_SourcePath(createInfo.sourcePath)
	{
		UseContext();

		auto shaderCode = Shader::ReadShaderFile(m_SourcePath);

		const VkShaderModuleCreateInfo shaderCreateInfo{
			.sType	  = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = shaderCode.size(),
			.pCode	  = reinterpret_cast<const uint32_t*>(shaderCode.data())
		};

		VkShaderModule shaderModule{};

		if (vkCreateShaderModule(ctx.m_LogicalDevice, &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS)
			EN_ERROR("ComputeShader::ComputeShader() - Failed to create the compute shader module!");

		const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
			.sType		    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(createInfo.descriptorLayouts.size()),
			.pSetLayouts	= createInfo.descriptorLayouts.data(),

			.pushConstantRangeCount = static_cast<uint32_t>(createInfo.pushConstantRanges.size()),
			.pPushConstantRanges	= createInfo.pushConstantRanges.data()
		};

		if (vkCreatePipelineLayout(ctx.m_LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout))
			EN_ERROR("ComputeShader::ComputeShader() - Failed to create the compute pipeline layout");
		
		const VkComputePipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,

			.stage {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = shaderModule,

				.pName = "main",
			},

			.layout = m_PipelineLayout,
		};

		if (vkCreateComputePipelines(ctx.m_LogicalDevice, VK_NULL_HANDLE, 1U, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			throw std::runtime_error("failed to create compute pipeline");
		
		vkDestroyShaderModule(ctx.m_LogicalDevice, shaderModule, nullptr);
	}

	ComputeShader::~ComputeShader()
	{
		UseContext();
		vkDestroyPipeline(ctx.m_LogicalDevice, m_Pipeline, nullptr);
		vkDestroyPipelineLayout(ctx.m_LogicalDevice, m_PipelineLayout, nullptr);
	}

	void ComputeShader::Dispatch(VkCommandBuffer& cmd, const uint32_t& x, const uint32_t& y, const uint32_t& z)
	{
		vkCmdDispatch(cmd, x, y, z);
	}
}
