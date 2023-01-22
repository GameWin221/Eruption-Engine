#pragma once

#ifndef EN_MEMORYBUFFER_HPP
#define EN_MEMORYBUFFER_HPP

#include <Renderer/Image.hpp>

namespace en
{
    class MemoryBuffer
    {
    public:
        MemoryBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage vmaMemoryUsage);
        ~MemoryBuffer();

        void MapMemory(const void* memory, VkDeviceSize memorySize);

        void CopyTo(Handle<MemoryBuffer> dstBuffer, VkCommandBuffer cmd = VK_NULL_HANDLE);
        void CopyTo(Handle<Image> dstImage, VkCommandBuffer cmd = VK_NULL_HANDLE);

        void CopyTo(MemoryBuffer* dstBuffer, VkCommandBuffer cmd = VK_NULL_HANDLE);
        void CopyTo(Image* dstImage, VkCommandBuffer cmd = VK_NULL_HANDLE);


        void PipelineBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmdBuffer);

        const VkBuffer GetHandle() const { return m_Buffer; };

        const VkDeviceSize m_BufferSize;

    private:
        VkBuffer m_Buffer;
        VmaAllocation m_Allocation;
    };
}

#endif