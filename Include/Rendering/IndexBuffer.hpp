#pragma once

#ifndef EN_INDEXBUFFER_HPP
#define EN_INDEXBUFFER_HPP

#include "Utility/Helpers.hpp"

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

        const size_t& GetSize() const { return m_Size; };

    private:
        size_t m_Size;
    };
}

#endif