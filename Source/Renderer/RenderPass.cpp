#include "Core/EnPch.hpp"
#include "RenderPass.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	void RenderPass::CreateRenderPass(std::vector<Subpass> subpasses)
	{
		UseContext();

		m_Subpasses = subpasses;

		std::vector<VkAttachmentDescription> descriptions;
		std::vector<VkAttachmentReference>   references;
		std::vector<VkSubpassDescription>    subpassDescriptions;
		std::vector<VkSubpassDependency>     subpassDependencies;

		for (auto& subpass : m_Subpasses)
		{
			EN_SUCCESS("ASDWAEAWE");

			const bool hasDepthAttachment = (subpass.depthAttachment.format != VK_FORMAT_UNDEFINED);

			for (const auto& attachment : subpass.colorAttachments)
			{
				VkAttachmentDescription description{};
				description.format		   = attachment.format;
				description.samples		   = VK_SAMPLE_COUNT_1_BIT;
				description.loadOp		   = attachment.loadOp;
				description.storeOp		   = attachment.storeOp;
				description.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				description.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				description.initialLayout  = attachment.initialLayout;
				description.finalLayout    = attachment.finalLayout;

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
				depthDescription.format			= subpass.depthAttachment.format;
				depthDescription.samples		= VK_SAMPLE_COUNT_1_BIT;
				depthDescription.loadOp			= subpass.depthAttachment.loadOp;
				depthDescription.storeOp		= subpass.depthAttachment.storeOp;
				depthDescription.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				depthDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
				depthDescription.initialLayout  = subpass.depthAttachment.initialLayout;
				depthDescription.finalLayout    = subpass.depthAttachment.finalLayout;

				depthReference.attachment = subpass.depthAttachment.index;
				depthReference.layout	  = subpass.depthAttachment.refLayout;
			}

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount	= static_cast<uint32_t>(references.size());
			subpass.pColorAttachments		= references.data();
			subpass.pDepthStencilAttachment = (hasDepthAttachment) ? &depthReference : nullptr;
			subpassDescriptions.emplace_back(subpass);

			VkSubpassDependency dependency{};
			dependency.srcSubpass	 = VK_SUBPASS_EXTERNAL;
			dependency.dstSubpass	 = 0U;
			dependency.srcAccessMask = 0U;
			dependency.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			if (hasDepthAttachment)
			{
				dependency.srcStageMask  |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				dependency.dstStageMask  |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
				dependency.dstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}

			subpassDependencies.emplace_back(dependency);

			if (hasDepthAttachment)
				descriptions.emplace_back(depthDescription);
		}

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType		   = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
		renderPassInfo.pAttachments	   = descriptions.data();
		renderPassInfo.subpassCount	   = static_cast<uint32_t>(subpassDescriptions.size());
		renderPassInfo.pSubpasses	   = subpassDescriptions.data();
		renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
		renderPassInfo.pDependencies   = subpassDependencies.data();

		if (vkCreateRenderPass(ctx.m_LogicalDevice, &renderPassInfo, nullptr, &m_RenderPass) != VK_SUCCESS)
			EN_ERROR("RenderPass::CreateRenderPass() - Failed to create render pass!");
	}
	void RenderPass::CreateSyncSemaphore()
	{
		UseContext();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		if (vkCreateSemaphore(ctx.m_LogicalDevice, &semaphoreInfo, nullptr, &m_PassFinished) != VK_SUCCESS)
			EN_ERROR("RenderPass::CreateSyncSemaphore() - Failed to create sync objects!");
	}
	void RenderPass::Bind(VkCommandBuffer& commandBuffer, VkFramebuffer& framebuffer, VkExtent2D& extent, std::vector<VkClearValue>& clearValues)
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType			 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass		 = m_RenderPass;
		renderPassInfo.framebuffer		 = framebuffer;
		renderPassInfo.renderArea.offset = { 0U, 0U };
		renderPassInfo.renderArea.extent = extent;
		renderPassInfo.clearValueCount	 = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues		 = clearValues.data();

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	void RenderPass::NextSubpass(VkCommandBuffer& commandBuffer)
	{
		vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
	}
	void RenderPass::Unbind(VkCommandBuffer& commandBuffer)
	{
		vkCmdEndRenderPass(commandBuffer);
	}
	void RenderPass::Resize(VkExtent2D& extent)
	{
		Destroy();

		CreateRenderPass(m_Subpasses);
	}

	RenderPass::~RenderPass()
	{
		Destroy();
	}
	void RenderPass::Destroy()
	{
		UseContext();

		vkDestroySemaphore(ctx.m_LogicalDevice, m_PassFinished, nullptr);

		vkDestroyRenderPass(ctx.m_LogicalDevice, m_RenderPass, nullptr);
	}
}