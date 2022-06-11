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

		extern void CreateImage(VkImage& image, VkDeviceMemory& imageMemory, VkExtent2D size, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, uint32_t mipLevels = 1U);
		extern void CreateImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels = 1U);
		extern void DestroyImage(VkImage& image, VkDeviceMemory& memory);

		extern void TransitionImageLayout(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels = 1U, VkCommandBuffer cmdBuffer = VK_NULL_HANDLE);

		void CreateCommandPool(VkCommandPool& commandPool, VkCommandPoolCreateFlags commandPoolCreateFlags);
		void CreateCommandBuffers(VkCommandBuffer* commandBuffers, uint32_t commandBufferCount, VkCommandPool& commandPool);

		extern uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		
		template<typename T, typename O>
		T CastTo(O object)
		{
			return reinterpret_cast<T>(object);
		}	
	}
}

#endif