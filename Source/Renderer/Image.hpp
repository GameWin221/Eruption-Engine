#pragma once

#ifndef EN_IMAGE_HPP
#define EN_IMAGE_HPP

namespace en
{
	class Image
	{
	public:
		Image(const VkExtent2D& size, const VkFormat& format, const VkImageUsageFlags& usageFlags, const VkImageAspectFlags& aspectFlags, const VkImageLayout& initialLayout, const bool& genMipMaps = false);
		~Image();

		void SetData(void* data);

		void ChangeLayout(const VkImageLayout& newLayout, const VkCommandBuffer& cmd = VK_NULL_HANDLE);

		VkImage		m_Image		= VK_NULL_HANDLE;
		VkImageView	m_ImageView = VK_NULL_HANDLE;

		const VkExtent2D m_Size;

		const VkImageUsageFlags  m_UsageFlags;
		const VkImageAspectFlags m_AspectFlags;

		const VkFormat m_Format;

		const VkImageLayout& GetLayout()    const { return m_CurrentLayout; };
		const uint32_t&		 GetMipLevels() const { return m_MipLevels;		};

		const bool UsesMipMaps() const { return m_MipLevels > 1U; };

	private:
		void GenMipMaps();

		uint32_t m_MipLevels = 1U;

		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		const VkImageLayout m_InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkDeviceMemory m_Memory = VK_NULL_HANDLE;
	};
}

#endif