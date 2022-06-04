#include <Core/EnPch.hpp>
#include "Pipeline.hpp"

#include <Common/Helpers.hpp>
#include <Renderer/Buffers/VertexBuffer.hpp>

namespace en
{
	Pipeline::~Pipeline()
	{
		Destroy();
	}
	void Pipeline::Bind(VkCommandBuffer& commandBuffer, VkFramebuffer& framebuffer, VkExtent2D& extent, std::vector<VkClearValue>& clearValues)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_RenderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea.offset = { 0U, 0U };
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}
	void Pipeline::Unbind(VkCommandBuffer& commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer);
	}
	void Pipeline::Resize(VkExtent2D& extent)
	{
		UseContext();

		vkDestroyRenderPass(ctx.m_LogicalDevice, m_RenderPass, nullptr);
		vkDestroyPipelineLayout(ctx.m_LogicalDevice, m_Layout, nullptr);
		vkDestroyPipeline(ctx.m_LogicalDevice, m_Pipeline, nullptr);

		RenderPassInfo renderPassInfo{};
		renderPassInfo.colorAttachments = m_ColorAttachments;
		renderPassInfo.depthAttachment = m_DepthAttachment;

		CreateRenderPass(renderPassInfo);

		Shader vShader(m_VShaderPath, ShaderType::Vertex);
		Shader fShader(m_FShaderPath, ShaderType::Fragment);

		PipelineInfo pipelineInfo{};
		pipelineInfo.vShader = &vShader;
		pipelineInfo.fShader = &fShader;
		pipelineInfo.extent = extent;
		pipelineInfo.descriptorLayouts = m_DescriptorSetLayouts;
		pipelineInfo.pushConstantRanges = m_PushConstantRanges;
		pipelineInfo.useVertexBindings = m_UseVertexBindings;
		pipelineInfo.enableDepthTest = m_EnableDepthTest;
		pipelineInfo.enableDepthWrite = m_EnableDepthWrite;
		pipelineInfo.blendEnable = m_BlendEnable;
		pipelineInfo.compareOp = m_CompareOp;
		pipelineInfo.cullMode = m_CullMode;
		pipelineInfo.polygonMode = m_PolygonMode;

		CreatePipeline(pipelineInfo);
	}

	void Pipeline::CreateRenderPass(RenderPassInfo& renderPass)
	{
		UseContext();

		m_ColorAttachments = renderPass.colorAttachments;
		m_DepthAttachment = renderPass.depthAttachment;

		std::vector<VkAttachmentDescription> descriptions;
		std::vector<VkAttachmentReference>   references;

		const bool hasDepthAttachment = (renderPass.depthAttachment.format != VK_FORMAT_UNDEFINED);

		for (const auto& attachment : renderPass.colorAttachments)
		{
			VkAttachmentDescription description{};
			description.format = attachment.format;
			description.samples = VK_SAMPLE_COUNT_1_BIT;
			description.loadOp = attachment.loadOp;
			description.storeOp = attachment.storeOp;
			description.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			description.initialLayout = attachment.initialLayout;
			description.finalLayout = attachment.finalLayout;

			descriptions.emplace_back(description);

			VkAttachmentReference reference{};
			reference.attachment = attachment.index;
			reference.layout = attachment.refLayout;

			references.emplace_back(reference);
		}

		VkAttachmentDescription depthDescription{};
		VkAttachmentReference   depthReference{};

		if (hasDepthAttachment)
		{
			depthDescription.format = renderPass.depthAttachment.format;
			depthDescription.samples = VK_SAMPLE_COUNT_1_BIT;
			depthDescription.loadOp = renderPass.depthAttachment.loadOp;
			depthDescription.storeOp = renderPass.depthAttachment.storeOp;
			depthDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthDescription.initialLayout = renderPass.depthAttachment.initialLayout;
			depthDescription.finalLayout = renderPass.depthAttachment.finalLayout;

			depthReference.attachment = renderPass.depthAttachment.index;
			depthReference.layout = renderPass.depthAttachment.refLayout;
		}

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = static_cast<uint32_t>(references.size());
		subpass.pColorAttachments = references.data();
		subpass.pDepthStencilAttachment = (hasDepthAttachment) ? &depthReference : nullptr;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0U;
		dependency.srcAccessMask = 0U;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		if (hasDepthAttachment)
		{
			dependency.srcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		}

		std::vector<VkAttachmentDescription> attachments = descriptions;

		if (hasDepthAttachment)
			attachments.emplace_back(depthDescription);

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1U;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1U;
		renderPassInfo.pDependencies = &dependency;

		if (vkCreateRenderPass(ctx.m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreateRenderPass() - Failed to create render pass!");
	}
	void Pipeline::CreatePipeline(PipelineInfo& pipeline)
	{
		m_VShaderPath = pipeline.vShader->GetPath();
		m_FShaderPath = pipeline.fShader->GetPath();

		m_DescriptorSetLayouts = pipeline.descriptorLayouts;
		m_PushConstantRanges = pipeline.pushConstantRanges;

		m_UseVertexBindings = pipeline.useVertexBindings;
		m_EnableDepthTest = pipeline.enableDepthTest;
		m_EnableDepthWrite = pipeline.enableDepthWrite;
		m_BlendEnable = pipeline.blendEnable;

		m_CompareOp = pipeline.compareOp;

		m_CullMode = pipeline.cullMode;
		m_PolygonMode = pipeline.polygonMode;

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
		viewport.width = (float)pipeline.extent.width;
		viewport.height = (float)pipeline.extent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = { 0U, 0U };
		scissor.extent = pipeline.extent;

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

		std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments(m_ColorAttachments.size());

		for (int i = 0; i < m_ColorAttachments.size(); i++)
			colorBlendAttachments[i] = colorBlendAttachment;

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

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(pipeline.descriptorLayouts.size());
		pipelineLayoutInfo.pSetLayouts = pipeline.descriptorLayouts.data();
		pipelineLayoutInfo.pPushConstantRanges = pipeline.pushConstantRanges.data();
		pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(pipeline.pushConstantRanges.size());

		if (vkCreatePipelineLayout(ctx.m_LogicalDevice, &pipelineLayoutInfo, nullptr, &m_Layout) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreatePipeline() - Failed to create pipeline layout!");

		VkPipelineShaderStageCreateInfo shaderStages[] = { pipeline.vShader->m_ShaderInfo, pipeline.fShader->m_ShaderInfo };

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2U;
		pipelineInfo.pStages = shaderStages;

		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr;

		pipelineInfo.layout = m_Layout;
		pipelineInfo.renderPass = m_RenderPass;
		pipelineInfo.subpass = 0U;

		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
		pipelineInfo.basePipelineIndex = -1;

		if (vkCreateGraphicsPipelines(ctx.m_LogicalDevice, VK_NULL_HANDLE, 1U, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreatePipeline() - Failed to create pipeline!");
	}
	void Pipeline::CreateSyncSemaphore()
	{
		UseContext();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(ctx.m_LogicalDevice, &semaphoreInfo, nullptr, &m_PassFinished) != VK_SUCCESS)
			EN_ERROR("Pipeline::CreateSyncSemaphore - Failed to create sync objects!");
	}

	void Pipeline::Destroy()
	{
		UseContext();

		vkDestroySemaphore(ctx.m_LogicalDevice, m_PassFinished, nullptr);

		vkDestroyRenderPass(ctx.m_LogicalDevice, m_RenderPass, nullptr);

		vkDestroyPipelineLayout(ctx.m_LogicalDevice, m_Layout, nullptr);
		vkDestroyPipeline(ctx.m_LogicalDevice, m_Pipeline, nullptr);
	}
}