#pragma once

#ifndef EN_HELPERS_HPP
#define EN_HELPERS_HPP

#include <Renderer/Context.hpp>

namespace en
{
	namespace Helpers
	{
		VkCommandBuffer BeginSingleTimeGraphicsCommands();
		void EndSingleTimeGraphicsCommands(VkCommandBuffer commandBuffer);

		VkCommandBuffer BeginSingleTimeTransferCommands();
		void EndSingleTimeTransferCommands(VkCommandBuffer commandBuffer);
		
		void CreateImageView(const VkImage image, VkImageView& imageView, const VkImageViewType viewType, const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t layer = 0U, const uint32_t layerCount = 1U, const uint32_t mipLevels = 1U);

		void SimpleTransitionImageLayout(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t layerCount = 1U, const uint32_t mipLevels = 1U, const VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
		void TransitionImageLayout(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask, const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage, const uint32_t layer = 0U, const uint32_t layerCount = 1U, const uint32_t mipLevels = 1U, const VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
	}
}

#endif