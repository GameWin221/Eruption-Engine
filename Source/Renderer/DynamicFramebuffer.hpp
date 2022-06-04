#pragma once

#ifndef EN_DYNAMICFRAMEBUFFER_HPP
#define EN_DYNAMICFRAMEBUFFER_HPP

namespace en
{
	class DynamicFramebuffer
	{
	public:
		~DynamicFramebuffer();

		struct AttachmentInfo
		{
			VkFormat		   format			  = VK_FORMAT_UNDEFINED;
			VkImageAspectFlags imageAspectFlags   = VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageUsageFlags  imageUsageFlags    = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VkImageLayout      initialLayout	  = VK_IMAGE_LAYOUT_UNDEFINED;
		};

		struct Attachment
		{
			VkImage		   image	   = VK_NULL_HANDLE;
			VkDeviceMemory imageMemory = VK_NULL_HANDLE;
			VkImageView    imageView   = VK_NULL_HANDLE;
			VkFormat	   format	   = VK_FORMAT_UNDEFINED;

			void Destroy();
		};

		void CreateAttachments(std::vector<AttachmentInfo> attachmentInfos, uint32_t sizeX, uint32_t sizeY);
		void CreateSampler(VkFilter framebufferFiltering = VK_FILTER_LINEAR);

		void Destroy();

		VkSampler m_Sampler;

		std::vector<Attachment> m_Attachments;

	private:
		uint32_t m_SizeX = 0U;
		uint32_t m_SizeY = 0U;
	};
}

#endif