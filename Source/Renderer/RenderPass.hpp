#pragma once
#ifndef EN_RENDERPASS_HPP
#define EN_RENDERPASS_HPP

#include <Renderer/Context.hpp>

namespace en
{
	class RenderPass
	{
	public:
		struct Attachment
		{
			std::vector<VkImageView> imageViews{};

			VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

			VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkImageLayout refLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkImageLayout finalLayout	= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentLoadOp  loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			VkAttachmentLoadOp  stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		};

		RenderPass(const VkExtent2D extent, const std::vector<RenderPass::Attachment>& attachments, const std::vector<VkSubpassDependency>& dependencies = {});
		~RenderPass();

		void Begin(VkCommandBuffer cmd, uint32_t framebufferIndex = 0U);
		void End();

		VkRenderPass m_RenderPass{};

		std::vector<VkClearValue> m_ClearColors{};

		const uint32_t m_AttachmentCount{};

	private:
		std::vector<VkFramebuffer> m_Framebuffers{};

		VkCommandBuffer m_BoundCMD{};
		VkExtent2D m_Extent{};
	};
}

#endif