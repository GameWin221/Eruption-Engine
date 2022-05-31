#pragma once

#ifndef EN_HELPERS_HPP
#define EN_HELPERS_HPP

#include <Renderer/Context.hpp>

namespace en
{
	namespace Helpers
	{
		extern VkCommandBuffer BeginSingleTimeCommands();
		extern void EndSingleTimeCommands(VkCommandBuffer& commandBuffer);

		extern void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels = 1U);
		extern void CreateImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1U);
		extern void TransitionImageLayout(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1U, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
		extern void DestroyImage(VkImage& image, VkDeviceMemory& memory);

		void CreateCommandPool(VkCommandPool& commandPool, VkCommandPoolCreateFlags commandPoolCreateFlags);
		void CreateCommandBuffers(VkCommandBuffer* commandBuffers, uint32_t commandBufferCount, VkCommandPool& commandPool);

		extern uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphicsFamily;
			std::optional<uint32_t> presentFamily;

			bool IsComplete()
			{
				return graphicsFamily.has_value() && presentFamily.has_value();
			}
		};

		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice& device);
	}
}

#endif