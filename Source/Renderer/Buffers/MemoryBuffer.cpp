#include <Core/EnPch.hpp>
#include "MemoryBuffer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	MemoryBuffer::MemoryBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties) : m_BufferSize(size)
	{
        UseContext();

        const VkBufferCreateInfo bufferInfo{
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = m_BufferSize,
            .usage       = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        if (vkCreateBuffer(ctx.m_LogicalDevice, &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
            EN_ERROR("Failed to create a buffer handle!")

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(ctx.m_LogicalDevice, m_Buffer, &memRequirements);

        const VkMemoryAllocateInfo allocInfo{
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = Helpers::FindMemoryType(memRequirements.memoryTypeBits, properties)
        };

        if (vkAllocateMemory(ctx.m_LogicalDevice, &allocInfo, nullptr, &m_BufferMemory) != VK_SUCCESS)
            EN_ERROR("Failed to allocate buffer memory!");

        vkBindBufferMemory(ctx.m_LogicalDevice, m_Buffer, m_BufferMemory, 0U);
	}
    MemoryBuffer::~MemoryBuffer()
    {
        UseContext();

        vkDestroyBuffer(ctx.m_LogicalDevice, m_Buffer      , nullptr);
        vkFreeMemory   (ctx.m_LogicalDevice, m_BufferMemory, nullptr);
    }

    void MemoryBuffer::MapMemory(const void* memory, const VkDeviceSize& memorySize)
    {
        UseContext();

        void* data;

        vkMapMemory(ctx.m_LogicalDevice, m_BufferMemory, 0U, memorySize, 0U, &data);
        memcpy(data, memory, static_cast<size_t>(memorySize));
        vkUnmapMemory(ctx.m_LogicalDevice, m_BufferMemory);
    }
    void MemoryBuffer::CopyTo(MemoryBuffer* dstBuffer, VkCommandBuffer cmd)
    {
        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();

        const VkBufferCopy copyRegion{ .size = m_BufferSize };

        vkCmdCopyBuffer(commandBuffer, m_Buffer, dstBuffer->m_Buffer, 1U, &copyRegion);

        if(!cmd)
            Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::CopyTo(Image* dstImage, VkCommandBuffer cmd)
    {
        UseContext();

        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();

        const VkBufferImageCopy region{
            .imageSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel   = 0U,
                .layerCount = 1U,
            },

            .imageExtent{
                .width  = dstImage->m_Size.width,
                .height = dstImage->m_Size.height,
                .depth  = 1U
            }
        };

        vkCmdCopyBufferToImage(commandBuffer, m_Buffer, dstImage->m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region);

        if(!cmd)
            Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::PipelineBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmdBuffer)
    {
        Helpers::BufferPipelineBarrier(m_Buffer, m_BufferSize, srcAccessMask, dstAccessMask, srcStage, dstStage, cmdBuffer);
    }
}