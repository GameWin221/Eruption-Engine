#pragma once

#ifndef EN_MEMORYBUFFER_HPP
#define EN_MEMORYBUFFER_HPP

#include <Renderer/Image.hpp>

namespace en
{
    class MemoryBuffer
    {
    public:
        MemoryBuffer(const VkDeviceSize& size, const VkBufferUsageFlags& usage, const VkMemoryPropertyFlags& properties);
        ~MemoryBuffer();

        void MapMemory(const void* memory, const VkDeviceSize& memorySize);
        void CopyTo(MemoryBuffer* dstBuffer, VkCommandBuffer cmd = VK_NULL_HANDLE);
        void CopyTo(Image* dstImage, VkCommandBuffer cmd = VK_NULL_HANDLE);

        const VkBuffer& GetHandle() const { return m_Buffer; };

        const VkDeviceSize m_BufferSize;

    private:
        VkBuffer       m_Buffer;
        VkDeviceMemory m_BufferMemory;
    };
}

#endif