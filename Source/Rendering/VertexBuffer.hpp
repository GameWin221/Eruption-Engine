#pragma once

#ifndef VERTEXBUFFER_HPP
#define VERTEXBUFFER_HPP

#include "Utility/Helpers.hpp"

namespace en
{
    struct Vertex
    {
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec2 texcoord;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

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

            // Texture coordinates attribute in glm::vec2 (UV)
            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Vertex, texcoord);


            return attributeDescriptions;
        }

        Vertex& operator=(const Vertex& other)
        {
            pos      = other.pos;
            normal   = other.normal;
            texcoord = other.texcoord;
        }

        bool operator==(const Vertex& other) const 
        {
            return pos == other.pos && texcoord == other.texcoord;
        }
    };

    class VertexBuffer
    {
    public:
        VertexBuffer(const std::vector<Vertex>& vertexData);
        ~VertexBuffer();

        void Bind(VkCommandBuffer& commandBuffer);

        VkBuffer       m_Buffer;
        VkDeviceMemory m_BufferMemory;

        const size_t& GetSize() const { return m_Size; };

    private:
        size_t m_Size;
    };
}

namespace std {
    template<> struct hash<en::Vertex> {
        size_t operator()(en::Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^
                (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
                (hash<glm::vec2>()(vertex.texcoord) << 1);
        }
    };
}
#endif