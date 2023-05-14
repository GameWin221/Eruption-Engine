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

        void CopyInto(const void* memory, VkDeviceSize memorySize, uint32_t dstOffset = 0U, VkCommandBuffer cmd = VK_NULL_HANDLE);

        void CopyTo(Handle<MemoryBuffer> dstBuffer, VkDeviceSize sizeBytes, VkDeviceSize srcOffset = 0U, VkDeviceSize dstOffset = 0U, VkCommandBuffer cmd = VK_NULL_HANDLE);
        void CopyTo(Handle<Image> dstImage, VkCommandBuffer cmd = VK_NULL_HANDLE);

        void CopyTo(VkBuffer dstBuffer, VkDeviceSize sizeBytes, VkDeviceSize srcOffset = 0U, VkDeviceSize dstOffset = 0U, VkCommandBuffer cmd = VK_NULL_HANDLE);
        void CopyTo(VkImage dstImage, VkExtent3D extent, VkCommandBuffer cmd = VK_NULL_HANDLE);

        void PipelineBarrier(VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmdBuffer);

        void Resize(VkDeviceSize newSize, VkCommandBuffer cmd = VK_NULL_HANDLE);

        const VkBuffer GetHandle() const { return m_Buffer; };

        const VkDeviceSize GetSize() const { return m_BufferSize; };

    private:
        VkBuffer      m_Buffer;
        VmaAllocation m_Allocation;
        VkDeviceSize  m_BufferSize;

        const VkBufferUsageFlags m_BufferUsage;
        const VmaMemoryUsage m_MemoryUsage;
    };
}

#endif