#include <Core/EnPch.hpp>
#include "Pipeline.hpp"

#include <Common/Helpers.hpp>
#include <Renderer/Buffers/VertexBuffer.hpp>

namespace en
{
	Pipeline::Pipeline(const CreateInfo& pipeline)
	{
		UseContext();

		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
		};

		if (pipeline.useVertexBindings)
		{
			vertexInputInfo.vertexBindingDescriptionCount = 1U;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		}

		constexpr VkPipelineInputAssemblyStateCreateInfo inputAssembly{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
			.primitiveRestartEnable = VK_FALSE
		};

		constexpr VkViewport viewport{
			.x = 0.0f,
			.y = 0.0f,
			.width = 1.0f,
			.height = 1.0f,
			.minDepth = 0.0f,
			.maxDepth = 1.0f,
		};

		constexpr VkRect2D scissor{
			.offset = { 0U, 0U },
			.extent = { 1U, 1U }
		};

		VkPipelineViewportStateCreateInfo viewportState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1U,
			.pViewports = &viewport,
			.scissorCount = 1U,
			.pScissors = &scissor
		};

		VkPipelineRasterizationStateCreateInfo rasterizer{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.depthClampEnable = VK_FALSE,
			.rasterizerDiscardEnable = VK_FALSE,
			.polygonMode = pipeline.polygonMode,
			.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
			.depthBiasEnable = VK_FALSE,
			.lineWidth = 1.0f
		};

		constexpr VkPipelineMultisampleStateCreateInfo multisampling{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
			.sampleShadingEnable = VK_FALSE
		};

		VkPipelineColorBlendAttachmentState colorBlendAttachment{
			.blendEnable = pipeline.blendEnable,
			.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
			.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
			.colorBlendOp = VK_BLEND_OP_ADD,
			.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
			.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
			.alphaBlendOp = VK_BLEND_OP_ADD,
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
		};

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(pipeline.colorFormats.size());

		for (auto& colorBlend : colorBlendAttachments)
			colorBlend = colorBlendAttachment;

		VkPipelineColorBlendStateCreateInfo colorBlending{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.logicOpEnable = VK_FALSE,
			.logicOp = VK_LOGIC_OP_COPY,
			.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
			.pAttachments = colorBlendAttachments.data(),
			.blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
		};

		VkPipelineDepthStencilStateCreateInfo depthStencil{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = pipeline.enableDepthTest,
			.depthWriteEnable = pipeline.enableDepthWrite,
			.depthCompareOp = pipeline.compareOp,
			.depthBoundsTestEnable = VK_FALSE,
			.stencilTestEnable = VK_FALSE,
			.front = {},
			.back = {},
			.minDepthBounds = 0.0f,
			.maxDepthBounds = 1.0f
		};

		constexpr std::array<VkDynamicState, 3> pipelineDynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
			VK_DYNAMIC_STATE_CULL_MODE
		};

		VkPipelineDynamicStateCreateInfo dynamicState{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(pipelineDynamicStates.size()),
			.pDynamicStates = pipelineDynamicStates.data()
		};

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = static_cast<uint32_t>(pipeline.descriptorLayouts.size()),
			.pSetLayouts = pipeline.descriptorLayouts.data(),
			.pushConstantRangeCount = static_cast<uint32_t>(pipeline.pushConstantRanges.size()),
			.pPushConstantRanges = pipeline.pushConstantRanges.data()
		};

		if (vkCreatePipelineLayout(ctx.m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreatePipeline() - Failed to create pipeline layout!");

		std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

		std::unique_ptr<Shader> vShader;
		std::unique_ptr<Shader> fShader;  

		if (!pipeline.vShader.empty())
		{
			vShader = std::make_unique<Shader>(pipeline.vShader, ShaderType::Vertex);
			shaderStages.emplace_back(vShader->m_ShaderInfo);
		}
			
		if (!pipeline.fShader.empty())
		{
			fShader = std::make_unique<Shader>(pipeline.fShader, ShaderType::Fragment);
			shaderStages.emplace_back(fShader->m_ShaderInfo);
		}
			
		if (shaderStages.empty())
			EN_ERROR("Pipeline::CreatePipeline() - No shaders stages specified!");

		VkPipelineRenderingCreateInfoKHR pipelineKHRCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.colorAttachmentCount = static_cast<uint32_t>(pipeline.colorFormats.size()),
			.pColorAttachmentFormats = pipeline.colorFormats.data(),
			.depthAttachmentFormat = pipeline.depthFormat
		};

		VkGraphicsPipelineCreateInfo pipelineInfo{
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.pNext = &pipelineKHRCreateInfo,
			.stageCount = static_cast<uint32_t>(shaderStages.size()),
			.pStages = shaderStages.data(),

			.pVertexInputState = &vertexInputInfo,
			.pInputAssemblyState = &inputAssembly,
			.pViewportState = &viewportState,
			.pRasterizationState = &rasterizer,
			.pMultisampleState = &multisampling,
			.pDepthStencilState = &depthStencil,
			.pColorBlendState = &colorBlending,
			.pDynamicState = &dynamicState,

			.layout = m_Layout,
			.renderPass = VK_NULL_HANDLE,
			.subpass = 0U,

			.basePipelineHandle = VK_NULL_HANDLE,
			.basePipelineIndex = -1
		};

		if (vkCreateGraphicsPipelines(ctx.m_LogicalDevice, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreatePipeline() - Failed to create pipeline!");
	}
	Pipeline::~Pipeline()
	{
		UseContext();

		vkDestroyPipelineLayout(ctx.m_LogicalDevice, m_Layout, nullptr);
		vkDestroyPipeline(ctx.m_LogicalDevice, m_Pipeline, nullptr);
	}
	void Pipeline::BeginRendering(VkCommandBuffer commandBuffer, const BindInfo& info)
	{
		m_RenderingCommandBuffer = commandBuffer;

		std::vector<VkRenderingAttachmentInfoKHR> khrColorAttachments(info.colorAttachments.size());

		for (int i = 0; i < khrColorAttachments.size(); i++)
		{
			const Attachment& attachment = info.colorAttachments[i];

			khrColorAttachments[i] = {
				.sType		 = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
				.imageView	 = attachment.imageView,
				.imageLayout = attachment.imageLayout,
				.loadOp		 = attachment.loadOp,
				.storeOp	 = attachment.storeOp,
				.clearValue  = attachment.clearValue
			};
		}

		const VkRenderingAttachmentInfoKHR depthAttachment {
			.sType		 = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.imageView   = info.depthAttachment.imageView,
			.imageLayout = info.depthAttachment.imageLayout,
			.loadOp		 = info.depthAttachment.loadOp,
			.storeOp	 = info.depthAttachment.storeOp,
			.clearValue  = info.depthAttachment.clearValue
		};
		
		const VkRenderingInfoKHR renderingInfo {
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
			.renderArea = {{0U, 0U}, info.extent},
			.layerCount = 1U,
			.colorAttachmentCount = static_cast<uint32_t>(khrColorAttachments.size()),
			.pColorAttachments = khrColorAttachments.data(),
			.pDepthAttachment = ((depthAttachment.imageView) ? &depthAttachment : nullptr)
		};

		vkCmdBeginRendering(m_RenderingCommandBuffer, &renderingInfo);

		const VkViewport viewport {
			.width  = static_cast<float>(info.extent.width),
			.height = static_cast<float>(info.extent.height),
			.maxDepth = 1.0f
		};
		const VkRect2D scissor {
			.extent = info.extent
		};
		
		vkCmdSetCullMode(m_RenderingCommandBuffer, info.cullMode);
		vkCmdSetViewport(m_RenderingCommandBuffer, 0U, 1U, &viewport);
		vkCmdSetScissor(m_RenderingCommandBuffer, 0U, 1U, &scissor);

		vkCmdBindPipeline(m_RenderingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}
	void Pipeline::PushConstants(const void* data, uint32_t size, uint32_t offset, VkShaderStageFlags shaderStage)
	{
		vkCmdPushConstants(m_RenderingCommandBuffer, m_Layout, shaderStage, offset, size, data);
	}
	void Pipeline::BindDescriptorSet(DescriptorSet* descriptor, uint32_t index)
	{
		vkCmdBindDescriptorSets(m_RenderingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, index, 1U, &descriptor->m_DescriptorSet, 0U, nullptr);
	}
	void Pipeline::BindDescriptorSet(VkDescriptorSet descriptor, uint32_t index)
	{
		vkCmdBindDescriptorSets(m_RenderingCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Layout, index, 1U, &descriptor, 0U, nullptr);
	}
	void Pipeline::BindVertexBuffer(VertexBuffer* buffer)
	{
		constexpr VkDeviceSize zeroOffset = 0U;
		vkCmdBindVertexBuffers(m_RenderingCommandBuffer, 0U, 1U, &buffer->m_Buffer->m_Handle, &zeroOffset);
	}
	void Pipeline::BindIndexBuffer(IndexBuffer* buffer)
	{
		vkCmdBindIndexBuffer(m_RenderingCommandBuffer, buffer->m_Buffer->m_Handle, 0U, VK_INDEX_TYPE_UINT32);
	}
	void Pipeline::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, uint32_t firstInstance)
	{
		vkCmdDrawIndexed(m_RenderingCommandBuffer, indexCount, instanceCount, firstIndex, 0U, firstInstance);
	}
	void Pipeline::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
	{
		vkCmdDraw(m_RenderingCommandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
	}
	void Pipeline::EndRendering()
	{
		vkCmdEndRendering(m_RenderingCommandBuffer);
		m_RenderingCommandBuffer = VK_NULL_HANDLE;
	}
}