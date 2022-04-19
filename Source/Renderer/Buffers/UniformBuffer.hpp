#pragma once

#ifndef UNIFORMBUFFER_HPP
#define UNIFORMBUFFER_HPP

#include <Common/Helpers.hpp>

#include "../../EruptionEngine.ini"

namespace en
{
    class UniformBuffer
    {
    public:
        UniformBuffer();
        ~UniformBuffer();

        void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);

        glm::mat4 m_Model;
        glm::mat4 m_View;
        glm::mat4 m_Proj;

        static VkDescriptorSetLayout& GetUniformDescriptorLayout();

    private:
        void CreateDescriptorSet();

        VkDescriptorSet m_DescriptorSet;

        VkBuffer       m_Buffer;
        VkDeviceMemory m_BufferMemory;

        struct UniformBufferObject
        {
            alignas(16) glm::mat4 model = glm::mat4(1.0f);
            alignas(16) glm::mat4 view = glm::mat4(1.0f);
            alignas(16) glm::mat4 proj = glm::mat4(1.0f);
        } m_UBO;
    };
}
#endif