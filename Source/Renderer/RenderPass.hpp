#pragma once
#ifndef EN_RENDERPASS_HPP
#define EN_RENDERPASS_HPP

#include <Renderer/Context.hpp>
//#include <Renderer/Framebuffer.hpp>

namespace en
{
	struct RenderPassAttachment
	{
		VkImageView imageView = VK_NULL_HANDLE;

		VkFormat format = VK_FORMAT_UNDEFINED;

		VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout refLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentLoadOp  loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

		VkAttachmentLoadOp  stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		VkAttachmentStoreOp stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	};
	class RenderPass
	{
	public:
		RenderPass(
			const RenderPassAttachment& colorAttachment = {},
			const RenderPassAttachment& depthStencilAttachment = {},
			const std::vector<VkSubpassDependency>& dependencies = {}
		);

		~RenderPass();
		
		void Begin(VkCommandBuffer cmd, VkFramebuffer framebuffer, VkExtent2D extent, glm::vec4 clearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), float depthClearValue = 1.0f, uint32_t stencilClearValue = 0U);
		void End();

		const VkRenderPass GetHandle() const { return m_RenderPass; }

		const bool m_UsesDepthAttachment;
		const bool m_UsesColorAttachment;

	private:
		VkRenderPass m_RenderPass{};
		VkCommandBuffer m_BoundCMD{};
	};
}

#endif