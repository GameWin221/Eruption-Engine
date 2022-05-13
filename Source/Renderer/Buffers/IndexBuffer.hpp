#pragma once

#ifndef EN_INDEXBUFFER_HPP
#define EN_INDEXBUFFER_HPP

#include <Common/Helpers.hpp>

namespace en
{
    class IndexBuffer
    {
    public:
        IndexBuffer(const std::vector<uint32_t>& indexData);
        ~IndexBuffer();

        void Bind(VkCommandBuffer& commandBuffer);

        VkBuffer m_Buffer;
        VkDeviceMemory m_BufferMemory;

        const uint32_t& GetSize() const { return m_Size; };

    private:
        uint32_t m_Size;
    };
}

#endif