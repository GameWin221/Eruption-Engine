#pragma once

#ifndef EN_INDEXBUFFER_HPP
#define EN_INDEXBUFFER_HPP

#include <Renderer/Buffers/MemoryBuffer.hpp>

namespace en
{
    class IndexBuffer
    {
    public:
        IndexBuffer(const std::vector<uint32_t>& indices);

        void Bind(VkCommandBuffer& commandBuffer);

        const VkDeviceSize& GetBufferSize()   const { return m_Buffer->GetSize(); };
        const uint32_t&     GetIndicesCount() const { return m_IndicesCount;      };

    private:
        std::unique_ptr<MemoryBuffer> m_Buffer;

        uint32_t m_IndicesCount;
    };
}

#endif