#include "Image.hpp"

#include <Renderer/Buffers/MemoryBuffer.hpp>

namespace en
{
	Image::Image(VkExtent2D size, VkFormat format, VkImageUsageFlags usageFlags, VkImageAspectFlags aspectFlags, VkImageCreateFlags createFlags, VkImageLayout initialLayout, uint32_t layerCount, bool genMipMaps)
		: m_Size(size), m_Format(format), m_UsageFlags(usageFlags), m_AspectFlags(aspectFlags), m_InitialLayout(initialLayout), m_LayerCount(layerCount)
	{
		if(genMipMaps)
			m_MipLevelCount = static_cast<uint32_t>(std::floor(std::log2(std::max(m_Size.width, m_Size.height)))) + 1U;

		VkImageCreateInfo imageInfo{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.flags = createFlags,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = format,

			.extent {
				.width = size.width,
				.height = size.height,
				.depth = 1U,
			},

			.mipLevels = m_MipLevelCount,
			.arrayLayers = m_LayerCount,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = m_UsageFlags,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		const VmaAllocationCreateInfo allocationInfo{
			.usage = VMA_MEMORY_USAGE_GPU_ONLY
		};

		if (vmaCreateImage(Context::Get().m_Allocator, &imageInfo, &allocationInfo, &m_Image, &m_Allocation, nullptr) != VK_SUCCESS)
			EN_ERROR("Image::Image() - Failed to create an image!")

		VkImageViewType imageViewType{};
		if (createFlags & VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
			imageViewType = VK_IMAGE_VIEW_TYPE_CUBE;
		else if (m_LayerCount > 1U)
			imageViewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		else
			imageViewType = VK_IMAGE_VIEW_TYPE_2D;

		Helpers::CreateImageView(m_Image, m_ImageView, imageViewType, m_Format, m_AspectFlags, 0U, m_LayerCount, m_MipLevelCount);

		if (m_LayerCount > 1U)
		{
			for (uint32_t i = 0U; i < m_LayerCount; i++)
			{
				VkImageView view{};
				Helpers::CreateImageView(m_Image, view, VK_IMAGE_VIEW_TYPE_2D, m_Format, m_AspectFlags, i, 1U, m_MipLevelCount);
				m_LayerImageViews.push_back(view);
			}
		}

		Helpers::SimpleTransitionImageLayout(m_Image, m_Format, m_AspectFlags, m_CurrentLayout, m_InitialLayout, m_LayerCount, m_MipLevelCount);
		m_CurrentLayout = m_InitialLayout;
	}
	Image::~Image()
	{
		UseContext();

		for(auto& layer : m_LayerImageViews)
			vkDestroyImageView(ctx.m_LogicalDevice, layer, nullptr);

		if (m_ImageView != VK_NULL_HANDLE)
			vkDestroyImageView(ctx.m_LogicalDevice, m_ImageView, nullptr);

		if (m_Allocation != VK_NULL_HANDLE && m_Image != VK_NULL_HANDLE)
			vmaDestroyImage(ctx.m_Allocator, m_Image, m_Allocation);
	}

	void Image::SetData(void* data)
	{
		VkDeviceSize imageByteSize = m_Size.width * m_Size.height * 4U;

		ChangeLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		MemoryBuffer stagingBuffer(
			imageByteSize, 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
			VMA_MEMORY_USAGE_CPU_TO_GPU
		);
		stagingBuffer.MapMemory(data, imageByteSize);
		stagingBuffer.CopyTo(m_Image, VkExtent3D{m_Size.width, m_Size.height, 1U});

		if(UsesMipMaps())
			GenMipMaps();
		else
		{
			Helpers::SimpleTransitionImageLayout(m_Image, m_Format, m_AspectFlags, m_CurrentLayout, m_InitialLayout, m_LayerCount, m_MipLevelCount);
			m_CurrentLayout = m_InitialLayout;
		}
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
				.layerCount = m_LayerCount,
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
		default:
			EN_ERROR("Image::GenMipMaps() - Unknown image layout in mipmap generation!");
			break;
		}

		for (uint32_t i = 1U; i < m_MipLevelCount; i++)
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
			blit.srcSubresource.layerCount = m_LayerCount;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0U;
			blit.dstSubresource.layerCount = m_LayerCount;

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

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = m_MipLevelCount - 1U;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = m_InitialLayout;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = accessFlags;

		vkCmdPipelineBarrier(cmd,
			VK_PIPELINE_STAGE_TRANSFER_BIT, stageFlags, 0U,
			0U, nullptr,
			0U, nullptr,
			1U, &barrier
		);

		m_CurrentLayout = m_InitialLayout;

		Helpers::EndSingleTimeGraphicsCommands(cmd);
	}

	void Image::ChangeLayout(VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd)
	{
		Helpers::TransitionImageLayout(m_Image, m_Format, m_AspectFlags, m_CurrentLayout, newLayout, srcAccessMask, dstAccessMask, srcStage, dstStage, 0U, 1U, m_MipLevelCount, cmd);
		m_CurrentLayout = newLayout;
	}
}