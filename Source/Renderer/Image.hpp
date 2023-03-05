#pragma once

#ifndef EN_IMAGE_HPP
#define EN_IMAGE_HPP

#include <Common/Helpers.hpp>
#include <Core/Types.hpp>

namespace en
{
	class Image
	{
	public:
		Image(VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VkImageLayout initialLayout, bool genMipMaps = false);
		Image(VkImage existingImage, VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VkImageLayout initialLayout);
		~Image();

		void Fill(glm::vec4 color, VkCommandBuffer cmd = VK_NULL_HANDLE);
		void SetData(void* data);
		/*
		void CopyTo(Image* dstImage, const VkCommandBuffer& cmd);

		void BlitTo(Image* dstImage, const VkFilter& filter, const VkCommandBuffer& cmd);
		*/
		void ChangeLayout(VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd = VK_NULL_HANDLE);

		const VkExtent2D m_Size{};

		const VkImageUsageFlags  m_UsageFlags{};
		const VkImageAspectFlags m_AspectFlags{};

		const VkFormat m_Format{};

		const VkImage	  GetHandle() const { return m_Image; };
		const VkImageView GetViewHandle() const { return m_ImageView; };

		const VkImageLayout GetLayout()    const { return m_CurrentLayout; };
		const uint32_t		GetMipLevels() const { return m_MipLevels;	   };

		const bool UsesMipMaps() const { return m_MipLevels > 1U; };

	private:
		void GenMipMaps();

		uint32_t m_MipLevels = 1U;

		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		const VkImageLayout m_InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		const bool m_CreatedFromExisting;

		VkImage		m_Image = VK_NULL_HANDLE;
		VkImageView	m_ImageView = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;
	};
}

#endif