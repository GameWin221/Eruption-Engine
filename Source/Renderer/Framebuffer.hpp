#pragma once

#ifndef EN_FRAMEBUFFER_HPP
#define EN_FRAMEBUFFER_HPP

namespace en
{
	class Framebuffer
	{
	public:
		struct AttachmentInfo
		{
			VkImageView imageView = VK_NULL_HANDLE;
			VkSampler   imageSampler = VK_NULL_HANDLE;
		};

	private:
		struct Attachment
		{
			VkImage		   image;
			VkDeviceMemory imageMemory;
			VkImageView    imageView;
			VkFormat	   format;

			void Destroy();
		};
		
		std::vector<Attachment> m_Attachments;

		VkFramebuffer m_Framebuffer;
		VkSampler	  m_Sampler;
	};
}

#endif