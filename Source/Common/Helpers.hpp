#pragma once

#ifndef EN_HELPERS_HPP
#define EN_HELPERS_HPP

#include <Renderer/Context.hpp>

namespace en
{
	namespace Helpers
	{
		VkCommandBuffer BeginSingleTimeGraphicsCommands();
		void EndSingleTimeGraphicsCommands(VkCommandBuffer& commandBuffer);

		VkCommandBuffer BeginSingleTimeTransferCommands();
		void EndSingleTimeTransferCommands(VkCommandBuffer& commandBuffer);
		
		void CreateImage(VkImage& image, VkDeviceMemory& imageMemory, const VkExtent2D& size, const VkFormat& format, const VkImageTiling& tiling, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& properties, const uint32_t& layers = 1U, const uint32_t& mipLevels = 1U, const VkImageCreateFlags& flags = 0U);
		void CreateImageView(VkImage& image, VkImageView& imageView, const VkImageViewType& viewType, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const uint32_t& layer = 0U, const uint32_t& layerCount = 1U, const uint32_t& mipLevels = 1U);
		void DestroyImage(VkImage& image, VkDeviceMemory& memory);

		void CreateSampler(VkSampler& sampler, const VkFilter& filtering = VK_FILTER_LINEAR, const uint32_t& anisotropy = 0U, const float& maxLod = 0.0f, const float& mipLodBias = 0.0f);
		
		void SimpleTransitionImageLayout(VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const VkImageLayout& oldLayout, const VkImageLayout& newLayout, const uint32_t& mipLevels = 1U, const VkCommandBuffer& cmdBuffer = VK_NULL_HANDLE);
		void TransitionImageLayout(VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const VkImageLayout& oldLayout, const VkImageLayout& newLayout, const VkAccessFlags& srcAccessMask, const VkAccessFlags& dstAccessMask, const VkPipelineStageFlags& srcStage, const VkPipelineStageFlags& dstStage, const uint32_t& layer = 0U, const uint32_t& layerCount = 1U, const uint32_t& mipLevels = 1U, const VkCommandBuffer& cmdBuffer = VK_NULL_HANDLE);

		void BufferPipelineBarrier(VkBuffer buffer, VkDeviceSize size, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmdBuffer);

		void CreateCommandPool(VkCommandPool& commandPool, const VkCommandPoolCreateFlags& commandPoolCreateFlags);
		void CreateCommandBuffers(VkCommandBuffer* commandBuffers, const uint32_t& commandBufferCount, VkCommandPool& commandPool);

		extern uint32_t FindMemoryType(const uint32_t& typeFilter, const VkMemoryPropertyFlags& properties);
	}
}

#endif