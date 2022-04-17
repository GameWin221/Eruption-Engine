#pragma once

#ifndef UNIFORMBUFFER_HPP
#define UNIFORMBUFFER_HPP

#include <Common/Helpers.hpp>

namespace en
{
    struct UniformBufferObject 
    {
        alignas(16) glm::mat4 model = glm::mat4(1.0f);
        alignas(16) glm::mat4 view  = glm::mat4(1.0f);
        alignas(16) glm::mat4 proj  = glm::mat4(1.0f);
    };

    class UniformBuffer
    {
    public:
        UniformBuffer();
        ~UniformBuffer();

        void Bind();

        UniformBufferObject m_UBO;

        VkBuffer       m_Buffer;
        VkDeviceMemory m_BufferMemory;
        VkDeviceSize   m_BufferSize;
    };
}
#endif