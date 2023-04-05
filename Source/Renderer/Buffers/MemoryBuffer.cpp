#include "MemoryBuffer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	MemoryBuffer::MemoryBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaMemoryUsage) 
        : m_BufferSize(size), m_BufferUsage(usage), m_MemoryUsage(vmaMemoryUsage)
	{
        UseContext();

        const VkBufferCreateInfo bufferInfo {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = m_BufferSize,
            .usage       = m_BufferUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo allocationInfo{
            .usage = m_MemoryUsage
        };

        if (vmaCreateBuffer(ctx.m_Allocator, &bufferInfo, &allocationInfo, &m_Buffer, &m_Allocation, nullptr) != VK_SUCCESS)
            EN_ERROR("MemoryBuffer::MemoryBuffer() - Failed to create a buffer!");
	}
    MemoryBuffer::~MemoryBuffer()
    {
        vmaDestroyBuffer(Context::Get().m_Allocator, m_Buffer, m_Allocation);
    }

    void MemoryBuffer::MapMemory(const void* memory, VkDeviceSize memorySize)
    {
        UseContext();

        void* data;

        vmaMapMemory(ctx.m_Allocator, m_Allocation, &data);
        memcpy(data, memory, static_cast<size_t>(memorySize));
        vmaUnmapMemory(ctx.m_Allocator, m_Allocation);
    }
    void MemoryBuffer::CopyInto(const void* memory, VkDeviceSize memorySize, uint32_t dstOffset, VkCommandBuffer cmd)
    {
        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();
        
        MemoryBuffer stagingBuffer(
            memorySize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        stagingBuffer.MapMemory(memory, memorySize);

        stagingBuffer.CopyTo(m_Buffer, memorySize, 0U, dstOffset, cmd);

        if (!cmd)
            Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::CopyTo(Handle<MemoryBuffer> dstBuffer, VkDeviceSize sizeBytes, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkCommandBuffer cmd)
    {
        CopyTo(dstBuffer->GetHandle(), sizeBytes, srcOffset, dstOffset, cmd);
    }
    void MemoryBuffer::CopyTo(Handle<Image> dstImage, VkCommandBuffer cmd)
    {
        CopyTo(dstImage, cmd);
    }
    void MemoryBuffer::CopyTo(VkBuffer dstBuffer, VkDeviceSize sizeBytes, VkDeviceSize srcOffset, VkDeviceSize dstOffset, VkCommandBuffer cmd)
    {
        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();

        VkBufferCopy copyRegion{
            .srcOffset = srcOffset,
            .dstOffset = dstOffset,
            .size = sizeBytes,
        };

        vkCmdCopyBuffer(commandBuffer, m_Buffer, dstBuffer, 1U, &copyRegion);

        if (!cmd)
            Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::CopyTo(VkImage dstImage, VkExtent3D extent, VkCommandBuffer cmd)
    {
        UseContext();

        VkCommandBuffer commandBuffer = cmd ? cmd : Helpers::BeginSingleTimeTransferCommands();

        const VkBufferImageCopy region{
            .imageSubresource{
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = 0U,
                .layerCount = 1U,
            },

            .imageExtent = extent
        };

        vkCmdCopyBufferToImage(commandBuffer, m_Buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region);

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
    void MemoryBuffer::Resize(VkDeviceSize newSize, VkCommandBuffer cmd)
    {
        VkBuffer newBuffer = VK_NULL_HANDLE;
        VmaAllocation newAllocation = VK_NULL_HANDLE;

        const VkBufferCreateInfo bufferInfo{
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = newSize,
            .usage       = m_BufferUsage,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE
        };

        const VmaAllocationCreateInfo allocationInfo{
            .usage = m_MemoryUsage
        };

        if (vmaCreateBuffer(Context::Get().m_Allocator, &bufferInfo, &allocationInfo, &newBuffer, &newAllocation, nullptr) != VK_SUCCESS)
            EN_ERROR("MemoryBuffer::MemoryBuffer() - Failed to create a buffer!");
    
        CopyTo(newBuffer, std::min(m_BufferSize, newSize), 0U, 0U, cmd);

        vmaDestroyBuffer(Context::Get().m_Allocator, m_Buffer, m_Allocation);

        m_Buffer     = newBuffer;
        m_Allocation = newAllocation;
        m_BufferSize = newSize;
    }
}