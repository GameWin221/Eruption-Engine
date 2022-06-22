#include "Core/EnPch.hpp"
#include "Image.hpp"

#include <Common/Helpers.hpp>
#include <Renderer/Buffers/MemoryBuffer.hpp>

namespace en
{
	Image::Image(const VkExtent2D& size, const VkFormat& format, const VkImageUsageFlags& usageFlags, const VkImageAspectFlags& aspectFlags, const VkImageLayout& initialLayout, const bool& genMipMaps) : m_Size(size), m_Format(format), m_UsageFlags(usageFlags), m_AspectFlags(aspectFlags), m_InitialLayout(initialLayout)
	{
		if(genMipMaps)
			m_MipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Size.width, m_Size.height)))) + 1U;

		Helpers::CreateImage(m_Image, m_Memory, m_Size, m_Format, VK_IMAGE_TILING_OPTIMAL, m_UsageFlags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_MipLevels);
		Helpers::CreateImageView(m_Image, m_ImageView, m_Format, m_AspectFlags, m_MipLevels);

		ChangeLayout(m_InitialLayout);
	}
	Image::~Image()
	{
		UseContext();

		if(m_Memory != VK_NULL_HANDLE)
			vkFreeMemory(ctx.m_LogicalDevice, m_Memory, nullptr);

		if (m_ImageView != VK_NULL_HANDLE)
			vkDestroyImageView(ctx.m_LogicalDevice, m_ImageView, nullptr);

		if (m_Image != VK_NULL_HANDLE)
			vkDestroyImage(ctx.m_LogicalDevice, m_Image, nullptr);
	}

	void Image::SetData(void* data)
	{
		VkDeviceSize imageByteSize = m_Size.width * m_Size.height * 4U;

		ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		MemoryBuffer stagingBuffer(imageByteSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
		stagingBuffer.MapMemory(data, imageByteSize);
		stagingBuffer.CopyTo(this);

		if(UsesMipMaps())
			GenMipMaps();
		else
			ChangeLayout(m_InitialLayout);
	}

	void Image::GenMipMaps()
	{
		UseContext();

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(ctx.m_PhysicalDevice, m_Format, &formatProperties);

		if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
			throw std::runtime_error("Image::GenMipMaps() - The specified image format does not support linear blitting!");

		VkCommandBuffer cmd = Helpers::BeginSingleTimeGraphicsCommands();

		VkImageMemoryBarrier barrier{
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

			.image = m_Image,

			.subresourceRange{
				.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
				.levelCount = 1U,
				.baseArrayLayer = 0U,
				.layerCount = 1U,
			}
		};

		int32_t mipWidth  = static_cast<int32_t>(m_Size.width);
		int32_t mipHeight = static_cast<int32_t>(m_Size.height);

		VkAccessFlags accessFlags{};
		VkPipelineStageFlags stageFlags{};

		switch (m_InitialLayout)
		{
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			accessFlags = VK_ACCESS_SHADER_READ_BIT;
			stageFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
			accessFlags = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			stageFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL || VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			accessFlags = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			stageFlags = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		default:
			EN_ERROR("Image::GenMipMaps() - Unknown image layout in mipmap generation!");
			break;
		}

		for (uint32_t i = 1U; i < m_MipLevels; i++)
		{
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout	  = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout	  = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0U,
				0U, nullptr,
				0U, nullptr,
				1U, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1U;
			blit.srcSubresource.baseArrayLayer = 0U;
			blit.srcSubresource.layerCount = 1U;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0U;
			blit.dstSubresource.layerCount = 1U;

			vkCmdBlitImage(cmd,
				m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1U, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = m_InitialLayout;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = accessFlags;

			vkCmdPipelineBarrier(cmd,
				VK_PIPELINE_STAGE_TRANSFER_BIT, stageFlags, 0U,
				0U, nullptr,
				0U, nullptr,
				1U, &barrier);

			if (mipWidth > 1U) mipWidth /= 2U;
			if (mipHeight > 1U) mipHeight /= 2U;
		}

		barrier.subresourceRange.baseMipLevel = m_MipLevels - 1U;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = m_InitialLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = accessFlags;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, stageFlags, 0U,
			0U, nullptr,
			0U, nullptr,
			1U, &barrier);

		m_CurrentLayout = m_InitialLayout;

		Helpers::EndSingleTimeGraphicsCommands(cmd);
	}

	void Image::ChangeLayout(const VkImageLayout& newLayout, const VkCommandBuffer& cmd)
	{
		Helpers::TransitionImageLayout(m_Image, m_Format, m_AspectFlags, m_CurrentLayout, newLayout, m_MipLevels, cmd);
		m_CurrentLayout = newLayout;
	}
}