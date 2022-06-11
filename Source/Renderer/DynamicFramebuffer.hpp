#pragma once

#ifndef EN_DYNAMICFRAMEBUFFER_HPP
#define EN_DYNAMICFRAMEBUFFER_HPP

#include <Renderer/Image.hpp>

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

		void CreateAttachments(std::vector<AttachmentInfo> attachmentInfos, VkExtent2D size);
		void CreateSampler(VkFilter framebufferFiltering = VK_FILTER_LINEAR);

		void Destroy();

		VkSampler m_Sampler;

		std::vector<Image> m_Attachments;

	private:
		VkExtent2D m_Size;
	};
}

#endif