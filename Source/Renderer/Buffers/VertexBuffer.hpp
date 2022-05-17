#pragma once

#ifndef EN_VERTEXBUFFER_HPP
#define EN_VERTEXBUFFER_HPP

#include <Renderer/Buffers/MemoryBuffer.hpp>

namespace en
{
    struct Vertex;

    class VertexBuffer
    {
    public:
        VertexBuffer(const std::vector<Vertex>& vertices);

        void Bind(VkCommandBuffer& commandBuffer);

        const VkDeviceSize& GetBufferSize()    const { return m_Buffer->GetSize(); };
        const uint32_t&     GetVerticesCount() const { return m_VerticesCount; };

    private:
        std::unique_ptr<MemoryBuffer> m_Buffer;

        uint32_t m_VerticesCount;
    };

    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 texcoord;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};

            // Position attribute in glm::vec3 (XYZ)
            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Vertex, pos);

            // Normal attribute in glm::vec3 (XYZ)
            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Vertex, normal);

            // Tangent attribute in glm::vec3 (XYZ)
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, tangent);

            // Texture coordinates attribute in glm::vec2 (UV)
            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Vertex, texcoord);


            return attributeDescriptions;
        }

        Vertex& operator=(const Vertex& other)
        {
            pos = other.pos;
            normal = other.normal;
            tangent = other.tangent;
            texcoord = other.texcoord;
            return *this;
        }

        bool operator==(const Vertex& other) const
        {
            return pos == other.pos && texcoord == other.texcoord && normal == other.normal && tangent == other.tangent;
        }
    };
}

#endif