#include <Core/EnPch.hpp>
#include "Pipeline.hpp"

#include <Common/Helpers.hpp>
#include <Renderer/Buffers/VertexBuffer.hpp>

namespace en
{
	static bool s_LoadedRenderingKHR = false;

	Pipeline::~Pipeline()
	{
		Destroy();
	}
	void Pipeline::Bind(VkCommandBuffer& commandBuffer, RenderingInfo& info)
	{
		std::vector<VkRenderingAttachmentInfoKHR> khrColorAttachments(info.colorAttachments.size());

		for (int i = 0; i < khrColorAttachments.size(); i++)
		{
			Attachment& attachment = info.colorAttachments[i];

			khrColorAttachments[i] =
			{
				.sType		 = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
				.imageView	 = attachment.imageView,
				.imageLayout = attachment.imageLayout,
				.loadOp		 = attachment.loadOp,
				.storeOp	 = attachment.storeOp,
				.clearValue  = attachment.clearValue
			};
		}

		VkRenderingAttachmentInfoKHR depthAttachment
		{
			.sType		 = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR,
			.imageView   = info.depthAttachment.imageView,
			.imageLayout = info.depthAttachment.imageLayout,
			.loadOp		 = info.depthAttachment.loadOp,
			.storeOp	 = info.depthAttachment.storeOp,
			.clearValue  = info.depthAttachment.clearValue
		};
		
		VkRenderingInfoKHR renderingInfo
		{
			.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR,
			.renderArea = {{0U, 0U}, info.extent},
			.layerCount = 1U,
			.colorAttachmentCount = static_cast<uint32_t>(khrColorAttachments.size()),
			.pColorAttachments = khrColorAttachments.data(),
			.pDepthAttachment = ((depthAttachment.imageView) ? &depthAttachment : nullptr)
		};

		vkCmdBeginRenderingKHR(commandBuffer, &renderingInfo);

		VkViewport viewport
		{
			.width  = static_cast<float>(info.extent.width),
			.height = static_cast<float>(info.extent.height),
			.maxDepth = 1.0f
		};
		VkRect2D scissor
		{
			.extent = info.extent
		};
		
		vkCmdSetViewport(commandBuffer, 0U, 1U, &viewport);

		vkCmdSetScissor(commandBuffer, 0U, 1U, &scissor);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}
	void Pipeline::Unbind(VkCommandBuffer& commandBuffer)
	{
		vkCmdEndRenderingKHR(commandBuffer);
	}
	void Pipeline::CreatePipeline(PipelineInfo& pipeline)
	{
		if (!m_Initialised)
		{
			VkInstance& instance = Context::Get().m_Instance;

			vkCmdBeginRenderingKHR = (PFN_vkCmdBeginRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdBeginRenderingKHR");
			vkCmdEndRenderingKHR = (PFN_vkCmdEndRenderingKHR)vkGetInstanceProcAddr(instance, "vkCmdEndRenderingKHR");

			if (!vkCmdBeginRenderingKHR || !vkCmdEndRenderingKHR)
				throw std::runtime_error("Pipeline::CreatePipeline() - Unable to load vkCmdBeginRenderingKHR and vkCmdEndRenderingKHR");
		}

		m_VShaderPath = pipeline.vShader->GetPath();
		m_FShaderPath = pipeline.fShader->GetPath();

		m_LastInfo = pipeline;

		UseContext();

		auto bindingDescription = Vertex::GetBindingDescription();
		auto attributeDescriptions = Vertex::GetAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		if (pipeline.useVertexBindings)
		{
			vertexInputInfo.vertexBindingDescriptionCount = 1U;
			vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
			vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
		}

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = 1.0f;
		viewport.height = 1.0f;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0U, 0U };
		scissor.extent.width = 1.0f;
		scissor.extent.height = 1.0f;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1U;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1U;
		viewportState.pScissors = &scissor;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = pipeline.polygonMode;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = pipeline.cullMode;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = pipeline.blendEnable;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(pipeline.colorFormats.size());

		for (auto& colorBlend : colorBlendAttachments)
			colorBlend = colorBlendAttachment;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
		colorBlending.pAttachments = colorBlendAttachments.data();
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = pipeline.enableDepthTest;
		depthStencil.depthWriteEnable = pipeline.enableDepthWrite;
		depthStencil.depthCompareOp = pipeline.compareOp;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f;
		depthStencil.maxDepthBounds = 1.0f;
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {};
		depthStencil.back = {};

		std::array<VkDynamicState, 2> pipelineDynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(pipelineDynamicStates.size());
		dynamicState.pDynamicStates = pipelineDynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(pipeline.descriptorLayouts.size());
		pipelineLayoutInfo.pSetLayouts = pipeline.descriptorLayouts.data();
		pipelineLayoutInfo.pPushConstantRanges = pipeline.pushConstantRanges.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pipeline.pushConstantRanges.size());

		if (vkCreatePipelineLayout(ctx.m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreatePipeline() - Failed to create pipeline layout!");

		VkPipelineShaderStageCreateInfo shaderStages[] = { pipeline.vShader->m_ShaderInfo, pipeline.fShader->m_ShaderInfo };

		std::vector<VkFormat> colorFormats;
		for (auto& format : pipeline.colorFormats)
			colorFormats.push_back(format);

		const VkPipelineRenderingCreateInfoKHR pipelineKHRCreateInfo{
			.sType					 = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
			.colorAttachmentCount	 = static_cast<uint32_t>(colorFormats.size()),
			.pColorAttachmentFormats = colorFormats.data(),
			.depthAttachmentFormat   = pipeline.depthFormat
		};

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType		= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2U;
		pipelineInfo.pStages	= shaderStages;

		pipelineInfo.pVertexInputState   = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState		 = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState	 = &multisampling;
		pipelineInfo.pDepthStencilState  = &depthStencil;
		pipelineInfo.pColorBlendState	 = &colorBlending;
		pipelineInfo.pDynamicState		 = &dynamicState;

		pipelineInfo.layout		= m_Layout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.pNext		= &pipelineKHRCreateInfo;
		pipelineInfo.subpass	= 0U;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex  = -1;

		if (vkCreateGraphicsPipelines(ctx.m_LogicalDevice, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreatePipeline() - Failed to create pipeline!");

		m_Initialised = true;
	}

	void Pipeline::Destroy()
	{
		UseContext();

		vkDestroyPipelineLayout(ctx.m_LogicalDevice, m_Layout, nullptr);
		vkDestroyPipeline(ctx.m_LogicalDevice, m_Pipeline, nullptr);
	}
}