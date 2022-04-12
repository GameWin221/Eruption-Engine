#pragma once

#ifndef EN_HELPERS_HPP
#define EN_HELPERS_HPP

#include <Rendering/Context.hpp>

namespace en
{
	namespace Helpers
	{
		extern VkCommandBuffer BeginSingleTimeCommands();
		extern void EndSingleTimeCommands(VkCommandBuffer& commandBuffer);

		extern void CopyBuffer(VkBuffer& srcBuffer, VkBuffer& dstBuffer, VkDeviceSize& size);
		extern void CreateBuffer(VkDeviceSize& size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		extern void MapBuffer(VkDeviceMemory& dstBufferMemory, const void* memory, VkDeviceSize& memorySize);
		extern void DestroyBuffer(VkBuffer& buffer, VkDeviceMemory& memory);

		extern void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		extern void CreateImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags);
		extern void TransitionImageLayout(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);
		extern void DestroyImage(VkImage& image, VkDeviceMemory& memory);

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