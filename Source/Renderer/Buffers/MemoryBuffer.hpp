#pragma once

#ifndef EN_MEMORYBUFFER_HPP
#define EN_MEMORYBUFFER_HPP

#include <Renderer/Image.hpp>

namespace en
{
    class MemoryBuffer
    {
    public:
        MemoryBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
        ~MemoryBuffer();

        void MapMemory(const void* memory, VkDeviceSize memorySize);
        void CopyTo(MemoryBuffer* dstBuffer, VkCommandBuffer cmd = VK_NULL_HANDLE);
        void CopyTo(Image* dstImage, VkCommandBuffer cmd = VK_NULL_HANDLE);

        void PipelineBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmdBuffer);

        const VkDeviceSize m_BufferSize;
        VkBuffer m_Handle;

    private:
        VkDeviceMemory m_BufferMemory;
    };
}

#endif