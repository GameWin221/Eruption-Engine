#include "MemoryBuffer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	MemoryBuffer::MemoryBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaMemoryUsage) : m_BufferSize(size)
	{
        UseContext();

        const VkBufferCreateInfo bufferInfo{
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = m_BufferSize,
            .usage       = usage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo allocationInfo{
            .usage = vmaMemoryUsage
        };

        vmaCreateBuffer(ctx.m_Allocator, &bufferInfo, &allocationInfo, &m_Buffer, &m_Allocation, nullptr);
	}
    MemoryBuffer::~MemoryBuffer()
    {
        UseContext();

        vmaDestroyBuffer(ctx.m_Allocator, m_Buffer, m_Allocation);
    }

    void MemoryBuffer::MapMemory(const void* memory, VkDeviceSize memorySize)
    {
        UseContext();

        void* data;

        vmaMapMemory(ctx.m_Allocator, m_Allocation, &data);
        memcpy(data, memory, static_cast<size_t>(memorySize));
        vmaUnmapMemory(ctx.m_Allocator, m_Allocation);
    }
    void MemoryBuffer::CopyTo(Handle<MemoryBuffer> dstBuffer, VkCommandBuffer cmd)
    {
        CopyTo(dstBuffer.get(), cmd);
    }
    void MemoryBuffer::CopyTo(Handle<Image> dstImage, VkCommandBuffer cmd)
    {
        CopyTo(dstImage, cmd);
    }
    void MemoryBuffer::CopyTo(MemoryBuffer* dstBuffer, VkCommandBuffer cmd)
    {
        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();

        const VkBufferCopy copyRegion{
            .size = m_BufferSize
        };

        vkCmdCopyBuffer(commandBuffer, m_Buffer, dstBuffer->m_Buffer, 1U, &copyRegion);

        if (!cmd)
            Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::CopyTo(Image* dstImage, VkCommandBuffer cmd)
    {
        UseContext();

        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();

        const VkBufferImageCopy region{
            .imageSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0U,
                .layerCount = 1U,
            },

            .imageExtent{
                .width = dstImage->m_Size.width,
                .height = dstImage->m_Size.height,
                .depth = 1U
            }
        };

        vkCmdCopyBufferToImage(commandBuffer, m_Buffer, dstImage->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region);

        if (!cmd)
            Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::PipelineBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmdBuffer)
    {
        VkBufferMemoryBarrier barrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,

                .srcAccessMask = srcAccessMask,
                .dstAccessMask = dstAccessMask,

                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,

                .buffer = m_Buffer,

                .offset = 0U,
                .size   = m_BufferSize,
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
}