#pragma once

#ifndef EN_VERTEXBUFFER_HPP
#define EN_VERTEXBUFFER_HPP

#include <Renderer/Buffers/MemoryBuffer.hpp>
#include <Renderer/Buffers/Vertex.hpp>

namespace en
{
    class VertexBuffer
    {
    public:
        VertexBuffer(const std::vector<Vertex>& vertices);

        std::unique_ptr<MemoryBuffer> m_Buffer;

        const uint32_t m_VerticesCount;
    };
}

#endif