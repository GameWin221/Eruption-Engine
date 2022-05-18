#pragma once

#ifndef EN_FRAMEBUFFER_HPP
#define EN_FRAMEBUFFER_HPP

namespace en
{
	class Framebuffer
	{
	public:
		~Framebuffer();

		struct AttachmentInfo
		{
			VkFormat		   format			= VK_FORMAT_UNDEFINED;
			VkImageLayout      imageLayout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageUsageFlags  imageUsageFlags  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		};
		struct Attachment
		{
			VkImage		   image	   = VK_NULL_HANDLE;
			VkDeviceMemory imageMemory = VK_NULL_HANDLE;
			VkImageView    imageView   = VK_NULL_HANDLE;
			VkFormat	   format	   = VK_FORMAT_UNDEFINED;

			void Destroy();
		};

		void CreateSampler();
		void CreateFramebuffer(VkRenderPass& renderPass);
		void CreateAttachments(std::vector<AttachmentInfo> attachmentInfos, uint32_t sizeX, uint32_t sizeY);

		void Destroy();

		std::vector<Attachment> m_Attachments;

		VkFramebuffer m_Framebuffer;
		VkSampler m_Sampler;

	private:
		uint32_t m_SizeX = 0U;
		uint32_t m_SizeY = 0U;
	};
}

#endif