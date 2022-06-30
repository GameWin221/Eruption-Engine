#include <Core/EnPch.hpp>
#include "SubMesh.hpp"

namespace en
{
	SubMesh::SubMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, Material* material)
		: m_Material(material), Asset{ AssetType::SubMesh }
	{
		m_VertexBuffer = std::make_unique<VertexBuffer>(vertices);
		m_IndexBuffer  = std::make_unique<IndexBuffer>(indices);
	}
	void SubMesh::Draw(VkCommandBuffer& cmd)
	{
		vkCmdDrawIndexed(cmd, m_IndexBuffer->m_IndicesCount, 1U, 0U, 0U, 0U);
	}
}