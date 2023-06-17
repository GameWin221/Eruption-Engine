#pragma once

#ifndef EN_IMAGE_HPP
#define EN_IMAGE_HPP

#include <Common/Helpers.hpp>
#include <Core/Types.hpp>

namespace en
{
	class Image
	{
		friend class Renderer;

	public:
		Image(VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VkImageCreateFlags createFlags, VkImageLayout initialLayout, uint32_t layerCount = 1U, bool genMipMaps = false);
		Image(VkImage image, VkImageView view, VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VkImageLayout layout, uint32_t layerCount = 1U);
		~Image();

		void SetData(void* data);

		void ChangeLayout(VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd = VK_NULL_HANDLE);

		const VkExtent2D m_Size{};

		const VkImageUsageFlags  m_UsageFlags{};
		const VkImageAspectFlags m_AspectFlags{};

		const VkFormat m_Format{};

		const uint32_t m_LayerCount{};

		const bool m_IsBorrowed;

		const VkImage	  GetHandle() const { return m_Image; };
		const VkImageView GetViewHandle() const { return m_ImageView; };

		const VkImageView GetLayerViewHandle(uint32_t layer) const { return m_LayerImageViews[layer]; };

		const VkImageLayout GetLayout()    const { return m_CurrentLayout; };
		const uint32_t		GetMipLevels() const { return m_MipLevelCount; };

		const bool UsesMipMaps() const { return m_MipLevelCount > 1U; };

	private:
		void GenMipMaps();

		uint32_t m_MipLevelCount = 1U;

		VkImageLayout m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		const VkImageLayout m_InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		VkImage		m_Image = VK_NULL_HANDLE;
		VkImageView	m_ImageView = VK_NULL_HANDLE;
		VmaAllocation m_Allocation = VK_NULL_HANDLE;

		std::vector<VkImageView> m_LayerImageViews{};
	};
}

#endif