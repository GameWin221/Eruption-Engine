#pragma once

#ifndef EN_RENDERPASS_HPP
#define EN_RENDERPASS_HPP

namespace en
{
	static std::vector<VkClearValue> s_DefaultBlackClearValue{ {0.0f, 0.0f, 0.0f, 1.0f} };

	class RenderPass
	{
		friend class Pipeline;

	public:
		struct Attachment
		{
			VkFormat format = VK_FORMAT_UNDEFINED;

			VkAttachmentLoadOp  loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp storeOp = VK_ATTACHMENT_STORE_OP_STORE;

			VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			VkImageLayout finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkImageLayout refLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			uint32_t index = 0U;
		};
		struct Subpass
		{
			std::vector<Attachment> colorAttachments{};
			Attachment depthAttachment{};
		};

		void CreateRenderPass(std::vector<Subpass> subpasses);
		void CreateSyncSemaphore();

		~RenderPass();

		void Bind(VkCommandBuffer& commandBuffer, VkFramebuffer& framebuffer, VkExtent2D& extent, std::vector<VkClearValue>& clearValues = s_DefaultBlackClearValue);

		void NextSubpass(VkCommandBuffer& commandBuffer);

		void Unbind(VkCommandBuffer& commandBuffer);

		void Resize(VkExtent2D& extent);

		void Destroy();

		VkRenderPass m_RenderPass;

		VkSemaphore m_PassFinished;

	private:
		std::vector<Subpass> m_Subpasses;
	};
}

#endif