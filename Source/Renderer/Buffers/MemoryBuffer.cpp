#include <Core/EnPch.hpp>
#include "MemoryBuffer.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	MemoryBuffer::MemoryBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties) : m_BufferSize(size)
	{
        UseContext();

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size        = m_BufferSize;
        bufferInfo.usage       = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(ctx.m_LogicalDevice, &bufferInfo, nullptr, &m_Buffer) != VK_SUCCESS)
            EN_ERROR("Failed to create a buffer handle!")


        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(ctx.m_LogicalDevice, m_Buffer, &memRequirements);


        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = Helpers::FindMemoryType(memRequirements.memoryTypeBits, properties);

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
    void MemoryBuffer::CopyTo(MemoryBuffer* dstBuffer)
    {
        UseContext();

        VkCommandBuffer commandBuffer = Helpers::BeginSingleTimeTransferCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = m_BufferSize;

        vkCmdCopyBuffer(commandBuffer, m_Buffer, dstBuffer->m_Buffer, 1U, &copyRegion);

        Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
    void MemoryBuffer::CopyTo(VkImage& dstImage, uint32_t width, uint32_t height)
    {
        UseContext();

        VkCommandBuffer commandBuffer = Helpers::BeginSingleTimeTransferCommands();

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel   = 0U;
        region.imageSubresource.layerCount = 1U;

        region.imageExtent = {
            width,
            height,
            1U
        };

        vkCmdCopyBufferToImage(commandBuffer, m_Buffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1U, &region);

        Helpers::EndSingleTimeTransferCommands(commandBuffer);
    }
}