#pragma once

#ifndef EN_DYNAMICFRAMEBUFFER_HPP
#define EN_DYNAMICFRAMEBUFFER_HPP

#include <Renderer/Image.hpp>

namespace en
{
	class DynamicFramebuffer
	{
	public:
		struct AttachmentInfo
		{
			VkFormat		   format			= VK_FORMAT_UNDEFINED;
			VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
			VkImageUsageFlags  imageUsageFlags  = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
			VkImageLayout      initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		};

		DynamicFramebuffer(const std::initializer_list<AttachmentInfo>& attachmentInfos, VkExtent2D size, VkFilter filtering = VK_FILTER_LINEAR);
		~DynamicFramebuffer();

		VkSampler m_Sampler;

		std::vector<Image> m_Attachments{};

		const VkExtent2D m_Size;
	};
}

#endif