#include "SubMesh.hpp"

namespace en
{
	SubMesh::SubMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Handle<Material> material)
		: m_VertexCount(vertices.size()), m_IndexCount(indices.size()), m_Material(material), Asset{AssetType::SubMesh}
	{
		m_VertexBuffer = MakeHandle<MemoryBuffer>(
			m_VertexCount * sizeof(Vertex), 
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_IndexBuffer = MakeHandle<MemoryBuffer>(
			m_IndexCount * sizeof(uint32_t),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VMA_MEMORY_USAGE_GPU_ONLY
		);

		m_VertexBuffer->CopyInto(vertices.data(), m_VertexBuffer->GetSize());
		m_IndexBuffer->CopyInto(indices.data(), m_IndexBuffer->GetSize());
	}
}