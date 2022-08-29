#pragma once

#ifndef EN_VERTEX_HPP
#define EN_VERTEX_HPP

namespace en 
{
	struct Vertex
	{
        glm::vec3 pos;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec2 texcoord;

        static constexpr VkVertexInputBindingDescription GetBindingDescription()
        {
            return VkVertexInputBindingDescription{
                .binding = 0U,
                .stride = sizeof(Vertex),
                .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
            };
        }

        static constexpr std::array<VkVertexInputAttributeDescription, 4> GetAttributeDescriptions()
        {
            return std::array<VkVertexInputAttributeDescription, 4> {
                VkVertexInputAttributeDescription{   // Position attribute in glm::vec3 (XYZ)
                    .location = 0U,
                    .binding = 0U,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, pos),
                },
                VkVertexInputAttributeDescription{   // Normal attribute in glm::vec3 (XYZ)
                    .location = 1U,
                    .binding = 0U,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, normal),
                },
                VkVertexInputAttributeDescription{   // Tangent attribute in glm::vec3 (XYZ)
                    .location = 2U,
                    .binding = 0U,
                    .format = VK_FORMAT_R32G32B32_SFLOAT,
                    .offset = offsetof(Vertex, tangent),
                },
                VkVertexInputAttributeDescription{    // Texture coordinates attribute in glm::vec2 (UV)
                    .location = 3U,
                    .binding = 0U,
                    .format = VK_FORMAT_R32G32_SFLOAT,
                    .offset = offsetof(Vertex, texcoord),
                }
            };
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