#include <Core/EnPch.hpp>
#include "Helpers.hpp"

namespace en
{
    namespace Helpers
    {
        VkCommandBuffer BeginSingleTimeGraphicsCommands()
        {
            UseContext();

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool        = ctx.m_CommandPool;
            allocInfo.commandBufferCount = 1U;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(ctx.m_LogicalDevice, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }
        void EndSingleTimeGraphicsCommands(VkCommandBuffer& commandBuffer)
        {
            UseContext();

            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1U;
            submitInfo.pCommandBuffers    = &commandBuffer;

            vkQueueSubmit(ctx.m_GraphicsQueue, 1U, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(ctx.m_GraphicsQueue);

            vkFreeCommandBuffers(ctx.m_LogicalDevice, ctx.m_CommandPool, 1U, &commandBuffer);
        }

        VkCommandBuffer BeginSingleTimeTransferCommands()
        {
            UseContext();

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

            allocInfo.commandPool = ctx.m_TransferCommandPool;
            allocInfo.commandBufferCount = 1U;
            
            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(ctx.m_LogicalDevice, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }
        void EndSingleTimeTransferCommands(VkCommandBuffer& commandBuffer)
        {
            UseContext();

            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1U;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(ctx.m_TransferQueue, 1U, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(ctx.m_TransferQueue);

            vkFreeCommandBuffers(ctx.m_LogicalDevice, ctx.m_TransferCommandPool, 1U, &commandBuffer);
        }

        void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory, uint32_t mipLevels)
        {
            UseContext();

            VkImageCreateInfo imageInfo{};
            imageInfo.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType     = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width  = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth  = 1U;
            imageInfo.mipLevels     = mipLevels;
            imageInfo.arrayLayers   = 1U;
            imageInfo.format        = format;
            imageInfo.tiling        = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage         = usage;
            imageInfo.samples       = VK_SAMPLE_COUNT_1_BIT;
            imageInfo.sharingMode   = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateImage(ctx.m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
                EN_ERROR("Failed to create image!");

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(ctx.m_LogicalDevice, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(ctx.m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
                EN_ERROR("Failed to allocate image memory!");

            vkBindImageMemory(ctx.m_LogicalDevice, image, imageMemory, 0);
        }
        void CreateImageView(VkImage& image, VkImageView& imageView, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
        {
            UseContext();

            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(ctx.m_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
                EN_ERROR("Failed to create texture image view!");
        }
        void TransitionImageLayout(VkImage& image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels, VkCommandBuffer cmdBuffer)
        {
            UseContext();

            VkCommandBuffer commandBuffer = cmdBuffer;

            bool usesTransferQueue = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.image = image;
            barrier.subresourceRange.aspectMask = aspectFlags;
            barrier.subresourceRange.baseMipLevel = 0U;
            barrier.subresourceRange.levelCount = mipLevels;
            barrier.subresourceRange.baseArrayLayer = 0U;
            barrier.subresourceRange.layerCount = 1U;

            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (cmdBuffer == VK_NULL_HANDLE)
            {
                if (usesTransferQueue)
                    commandBuffer = BeginSingleTimeTransferCommands();
                else
                    commandBuffer = BeginSingleTimeGraphicsCommands();
            }

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }

            // From color attachment to shader optimal
            else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            // From shader optimal to color attachment 
            else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }
            // From depth attachment to shader optimal
            else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else
                EN_ERROR("Unsupported layout transition!");

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            if (cmdBuffer == VK_NULL_HANDLE)
            {
                if (usesTransferQueue)
                    EndSingleTimeTransferCommands(commandBuffer);
                else
                    EndSingleTimeGraphicsCommands(commandBuffer);
            }
        }
        void DestroyImage(VkImage& image, VkDeviceMemory& memory)
        {
            UseContext();

            vkDestroyImage(ctx.m_LogicalDevice, image, nullptr);
            vkFreeMemory(ctx.m_LogicalDevice, memory, nullptr);
        }

        void CreateCommandPool(VkCommandPool& commandPool, VkCommandPoolCreateFlags commandPoolCreateFlags)
        {
            UseContext();

            VkCommandPoolCreateInfo commandPoolCreateInfo = {};
            commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            commandPoolCreateInfo.queueFamilyIndex = ctx.m_QueueFamilies.graphics.value();
            commandPoolCreateInfo.flags = commandPoolCreateFlags;

            if (vkCreateCommandPool(ctx.m_LogicalDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
                EN_ERROR("Context::VKCreateCommandPool() - Failed to create a command pool!");
        }

        void CreateCommandBuffers(VkCommandBuffer* commandBuffers, uint32_t commandBufferCount, VkCommandPool& commandPool)
        {
            UseContext();

            VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
            commandBufferAllocateInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            commandBufferAllocateInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            commandBufferAllocateInfo.commandPool        = commandPool;
            commandBufferAllocateInfo.commandBufferCount = commandBufferCount;
            vkAllocateCommandBuffers(ctx.m_LogicalDevice, &commandBufferAllocateInfo, commandBuffers);
        }
        
        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
        {
            UseContext();

            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(ctx.m_PhysicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }

            EN_ERROR("Failed to find suitable memory type!");
        }
    }
}