#include "ComputePipeline.hpp"

namespace en
{
	ComputePipeline::ComputePipeline(const CreateInfo& createInfo)
	{
		UseContext();

		VkShaderModule shaderModule = CreateShaderModule(createInfo.sourcePath);

		const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{
			.sType		    = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(createInfo.descriptorLayouts.size()),
			.pSetLayouts	= createInfo.descriptorLayouts.data(),
			
			.pushConstantRangeCount = static_cast<uint32_t>(createInfo.pushConstantRanges.size()),
			.pPushConstantRanges	= createInfo.pushConstantRanges.data()
		};

		if (vkCreatePipelineLayout(ctx.m_LogicalDevice, &pipelineLayoutCreateInfo, nullptr, &m_Layout))
			EN_ERROR("ComputePipeline::ComputePipeline() - Failed to create the compute pipeline layout!");
		
		const VkComputePipelineCreateInfo pipelineCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
			
			.stage {
				.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
				.stage = VK_SHADER_STAGE_COMPUTE_BIT,
				.module = shaderModule,
				
				.pName = "main",
			},

			.layout = m_Layout,
		};

		if (vkCreateComputePipelines(ctx.m_LogicalDevice, VK_NULL_HANDLE, 1U, &pipelineCreateInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			EN_ERROR("ComputePipeline::ComputePipeline() - Failed to create compute pipeline!");
		
		DestroyShaderModule(shaderModule);
	}

	void ComputePipeline::Bind(const VkCommandBuffer cmd)
	{
		m_BoundCommandBuffer = cmd;

		vkCmdBindPipeline(m_BoundCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_Pipeline);
	}

	void ComputePipeline::Dispatch(const uint32_t x, const uint32_t y, const uint32_t z)
	{
		vkCmdDispatch(m_BoundCommandBuffer, x, y, z);
	}
}
