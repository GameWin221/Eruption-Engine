#pragma once

#ifndef EN_HELPERS_HPP
#define EN_HELPERS_HPP

#include <Renderer/Context.hpp>

namespace en
{
	namespace Helpers
	{
		extern VkCommandBuffer BeginSingleTimeGraphicsCommands();
		extern void EndSingleTimeGraphicsCommands(VkCommandBuffer& commandBuffer);

		extern VkCommandBuffer BeginSingleTimeTransferCommands();
		extern void EndSingleTimeTransferCommands(VkCommandBuffer& commandBuffer);

		extern void CreateImage(VkImage& image, VkDeviceMemory& imageMemory, const VkExtent2D& size, const VkFormat& format, const VkImageTiling& tiling, const VkImageUsageFlags& usage, const VkMemoryPropertyFlags& properties, const uint32_t& mipLevels = 1U);
		extern void CreateImageView(VkImage& image, VkImageView& imageView, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const uint32_t& mipLevels = 1U);
		extern void DestroyImage(VkImage& image, VkDeviceMemory& memory);

		extern void CreateSampler(VkSampler& sampler, const VkFilter& filtering = VK_FILTER_LINEAR, const uint32_t& anisotropy = 0U, const float& maxLod = 0.0f, const float& mipLodBias = 0.0f);
		
		extern void TransitionImageLayout(VkImage& image, const VkFormat& format, const VkImageAspectFlags& aspectFlags, const VkImageLayout& oldLayout, const VkImageLayout& newLayout, const uint32_t& mipLevels = 1U, const VkCommandBuffer& cmdBuffer = VK_NULL_HANDLE);

		void CreateCommandPool(VkCommandPool& commandPool, const VkCommandPoolCreateFlags& commandPoolCreateFlags);
		void CreateCommandBuffers(VkCommandBuffer* commandBuffers, const uint32_t& commandBufferCount, VkCommandPool& commandPool);

		extern uint32_t FindMemoryType(const uint32_t& typeFilter, const VkMemoryPropertyFlags& properties);
	}
}

#endif