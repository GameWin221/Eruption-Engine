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

        std::unique_ptr<MemoryBuffer> m_Buffer;

        const uint32_t m_IndicesCount;
    };
}

#endif