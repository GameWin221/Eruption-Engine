#include <Core/EnPch.hpp>
#include "Helpers.hpp"

namespace en
{
    namespace Helpers
    {
        VkCommandBuffer BeginSingleTimeGraphicsCommands()
        {
            UseContext();

            VkCommandBufferAllocateInfo allocInfo{
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = ctx.m_CommandPool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1U,
            };

            VkCommandBuffer commandBuffer{};
            vkAllocateCommandBuffers(ctx.m_LogicalDevice, &allocInfo, &commandBuffer);

            constexpr VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            };

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }
        void EndSingleTimeGraphicsCommands(const VkCommandBuffer commandBuffer)
        {
            UseContext();

            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{
                .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1U,
                .pCommandBuffers    = &commandBuffer,
            };

            vkQueueSubmit(ctx.m_GraphicsQueue, 1U, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(ctx.m_GraphicsQueue);

            vkFreeCommandBuffers(ctx.m_LogicalDevice, ctx.m_CommandPool, 1U, &commandBuffer);
        }

        VkCommandBuffer BeginSingleTimeTransferCommands()
        {
            UseContext();

            VkCommandBufferAllocateInfo allocInfo {
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = ctx.m_TransferCommandPool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = 1U
            };
            
            VkCommandBuffer commandBuffer{};
            vkAllocateCommandBuffers(ctx.m_LogicalDevice, &allocInfo, &commandBuffer);

            constexpr VkCommandBufferBeginInfo beginInfo{
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
            };

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }
        void EndSingleTimeTransferCommands(const VkCommandBuffer commandBuffer)
        {
            UseContext();

            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo {
                .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .commandBufferCount = 1U,
                .pCommandBuffers    = &commandBuffer,
            };

            vkQueueSubmit(ctx.m_TransferQueue, 1U, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(ctx.m_TransferQueue);

            vkFreeCommandBuffers(ctx.m_LogicalDevice, ctx.m_TransferCommandPool, 1U, &commandBuffer);
        }

        void CreateImage(VkImage& image, VkDeviceMemory& imageMemory, const VkExtent2D size, const VkFormat format, const VkImageTiling tiling, const VkImageUsageFlags usage, const VkMemoryPropertyFlags properties, const uint32_t layers, const uint32_t mipLevels, const VkImageCreateFlags flags)
        {
            UseContext();

            VkImageCreateInfo imageInfo{
                .sType     = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
                .flags     = flags,
                .imageType = VK_IMAGE_TYPE_2D,
                .format    = format,

                .extent {
                    .width  = size.width,
                    .height = size.height,
                    .depth  = 1U,
                },

                .mipLevels     = mipLevels,
                .arrayLayers   = layers,
                .samples       = VK_SAMPLE_COUNT_1_BIT,
                .tiling        = tiling,
                .usage         = usage,
                .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
                .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            };

            if (vkCreateImage(ctx.m_LogicalDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
                EN_ERROR("Helpers::CreateImage() - Failed to create image!");

            VkMemoryRequirements memRequirements{};
            vkGetImageMemoryRequirements(ctx.m_LogicalDevice, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo {
                .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
                .allocationSize  = memRequirements.size,
                .memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties)
            };

            if (vkAllocateMemory(ctx.m_LogicalDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
                EN_ERROR("Helpers::CreateImage() - Failed to allocate image memory!");

            vkBindImageMemory(ctx.m_LogicalDevice, image, imageMemory, 0);
        }
        void CreateImageView(const VkImage image, VkImageView& imageView, const VkImageViewType viewType, const VkFormat format, const VkImageAspectFlags aspectFlags, const uint32_t layer, const uint32_t layerCount, const uint32_t mipLevels)
        {
            VkImageViewCreateInfo viewInfo {
                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = image,
                .viewType = viewType,
                .format   = format,

                .subresourceRange {
                    .aspectMask     = aspectFlags,
                    .baseMipLevel   = 0U,
                    .levelCount     = mipLevels,
                    .baseArrayLayer = layer,
                    .layerCount     = layerCount,
                }
            };

            if (vkCreateImageView(Context::Get().m_LogicalDevice, &viewInfo, nullptr, &imageView) != VK_SUCCESS)
                EN_ERROR("Helpers::CreateImageView() - Failed to create texture image view!");
        }
        
        void SimpleTransitionImageLayout(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, const VkImageLayout oldLayout, const VkImageLayout newLayout, const uint32_t mipLevels, const VkCommandBuffer cmdBuffer)
        {
            UseContext();

            bool usesTransferQueue = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            VkImageMemoryBarrier barrier{
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

                .oldLayout = oldLayout,
                .newLayout = newLayout,

                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

                .image = image,

                .subresourceRange {
                    .aspectMask = aspectFlags,
                    .baseMipLevel = 0U,
                    .levelCount = mipLevels,
                    .baseArrayLayer = 0U,
                    .layerCount = 1U,
                },
            };

            VkPipelineStageFlags sourceStage = 0U;
            VkPipelineStageFlags destinationStage = 0U;

            VkCommandBuffer commandBuffer{};

            if (cmdBuffer == VK_NULL_HANDLE)
            {
                if (usesTransferQueue)
                    commandBuffer = BeginSingleTimeTransferCommands();
                else
                    commandBuffer = BeginSingleTimeGraphicsCommands();
            }
            else
                commandBuffer = cmdBuffer;

            switch(oldLayout)
            {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                switch (newLayout)
                {
                case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

                    sourceStage     = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;
                case VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

                    sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                    destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                    break;
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;
                case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                    barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                    break;

                default:
                    EN_ERROR("Unsupported layout transition from VK_IMAGE_LAYOUT_UNDEFINED!"); 
                    break;
                }
                break;

            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                switch (newLayout)
                {
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
                    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                    sourceStage      = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                    break;

                default:
                    EN_ERROR("Unsupported layout transition from VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL!");
                    break;
                }
                break;
            
            default:
                EN_ERROR("Unsupported oldLayout!");
                break;
            }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage, destinationStage,
                0U,
                0U, nullptr,
                0U, nullptr,
                1U, &barrier
            );

            if (cmdBuffer == VK_NULL_HANDLE)
            {
                if (usesTransferQueue)
                    EndSingleTimeTransferCommands(commandBuffer);
                else
                    EndSingleTimeGraphicsCommands(commandBuffer);
            }
        }
        void TransitionImageLayout(const VkImage image, const VkFormat format, const VkImageAspectFlags aspectFlags, const VkImageLayout oldLayout, const VkImageLayout newLayout, const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask, const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage, const uint32_t layer, const uint32_t layerCount, const uint32_t mipLevels, const VkCommandBuffer cmdBuffer)
        {
            UseContext();

            bool usesTransferQueue = (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

            const VkImageMemoryBarrier barrier {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,

                .srcAccessMask = srcAccessMask,
                .dstAccessMask = dstAccessMask,

                .oldLayout = oldLayout,
                .newLayout = newLayout,

                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

                .image = image,
                
                .subresourceRange {
                    .aspectMask     = aspectFlags,
                    .baseMipLevel   = 0U,
                    .levelCount     = mipLevels,
                    .baseArrayLayer = layer,
                    .layerCount     = layerCount,
                }
            };

            VkCommandBuffer commandBuffer{};
            if (cmdBuffer == VK_NULL_HANDLE)
            {
                if (usesTransferQueue)
                    commandBuffer = BeginSingleTimeTransferCommands();
                else
                    commandBuffer = BeginSingleTimeGraphicsCommands();
            }
            else
                commandBuffer = cmdBuffer;

            vkCmdPipelineBarrier(
                commandBuffer,
                srcStage, dstStage,
                0U,
                0U, nullptr,
                0U, nullptr,
                1U, &barrier
            );

            if (cmdBuffer == VK_NULL_HANDLE)
            {
                if (usesTransferQueue)
                    EndSingleTimeTransferCommands(commandBuffer);
                else
                    EndSingleTimeGraphicsCommands(commandBuffer);
            }
        }

        void BufferPipelineBarrier(const VkBuffer buffer, const VkDeviceSize size, const VkAccessFlags srcAccessMask, const VkAccessFlags dstAccessMask, const VkPipelineStageFlags srcStage, const VkPipelineStageFlags dstStage, const VkCommandBuffer cmdBuffer)
        {
            VkBufferMemoryBarrier barrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,

                .srcAccessMask = srcAccessMask,
                .dstAccessMask = dstAccessMask,

                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

                .buffer = buffer,

                .offset = 0U,
                .size = size,
            };

            vkCmdPipelineBarrier(
                cmdBuffer,
                srcStage, dstStage,
                0U,
                0U, nullptr,
                1U, &barrier,
                0U, nullptr
            );
        }

        void DestroyImage(const VkImage image, const VkDeviceMemory memory)
        {
            UseContext();

            vkDestroyImage(ctx.m_LogicalDevice, image, nullptr);
            vkFreeMemory(ctx.m_LogicalDevice, memory, nullptr);
        }

        void CreateSampler(VkSampler& sampler, const VkFilter filtering, const uint32_t anisotropy, const float maxLod, const float mipLodBias)
        {
            UseContext();

            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(ctx.m_PhysicalDevice, &properties);

            VkSamplerCreateInfo samplerInfo{
                .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,

                .magFilter = filtering,
                .minFilter = filtering,

                .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,

                .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
                .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,

                .mipLodBias = mipLodBias,

                .anisotropyEnable = (anisotropy > 0),
                .maxAnisotropy = std::fminf(anisotropy, properties.limits.maxSamplerAnisotropy),

                .compareEnable = VK_FALSE,
                .compareOp = VK_COMPARE_OP_ALWAYS,

                .minLod = 0.0f,
                .maxLod = maxLod,

                .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
                .unnormalizedCoordinates = VK_FALSE,
            };

            if (vkCreateSampler(ctx.m_LogicalDevice, &samplerInfo, nullptr, &sampler) != VK_SUCCESS)
                EN_ERROR("Helpers::CreateSampler() - Failed to create texture sampler!");
        }

        void CreateCommandPool(VkCommandPool& commandPool, const VkCommandPoolCreateFlags commandPoolCreateFlags)
        {
            UseContext();

            VkCommandPoolCreateInfo commandPoolCreateInfo{
                .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
                .flags            = commandPoolCreateFlags,
                .queueFamilyIndex = ctx.m_QueueFamilies.graphics.value(),
            };

            if (vkCreateCommandPool(ctx.m_LogicalDevice, &commandPoolCreateInfo, nullptr, &commandPool) != VK_SUCCESS)
                EN_ERROR("Helpers::CreateCommandPool() - Failed to create a command pool!");
        }

        void CreateCommandBuffers(VkCommandBuffer* commandBuffers, const uint32_t commandBufferCount, VkCommandPool commandPool)
        {
            UseContext();

            VkCommandBufferAllocateInfo commandBufferAllocateInfo{
                .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
                .commandPool        = commandPool,
                .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
                .commandBufferCount = commandBufferCount,
            };

            vkAllocateCommandBuffers(ctx.m_LogicalDevice, &commandBufferAllocateInfo, commandBuffers);
        }
        
        uint32_t FindMemoryType(const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
        {
            UseContext();

            VkPhysicalDeviceMemoryProperties memProperties{};
            vkGetPhysicalDeviceMemoryProperties(ctx.m_PhysicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
            {
                if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
                    return i;
            }

            EN_ERROR("Helpers::FindMemoryType() - Failed to find suitable memory type!");
        }
    }
}