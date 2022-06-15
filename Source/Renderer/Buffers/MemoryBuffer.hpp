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

        void MapMemory(const void* memory, const VkDeviceSize& memorySize);
        void CopyTo(MemoryBuffer* dstBuffer);
        void CopyTo(Image* dstImage);

        const VkBuffer&     GetHandle() const { return m_Buffer;     };
        const VkDeviceSize& GetSize()   const { return m_BufferSize; };

    private:
        VkBuffer       m_Buffer;
        VkDeviceMemory m_BufferMemory;
        VkDeviceSize   m_BufferSize;
    };
}

#endif