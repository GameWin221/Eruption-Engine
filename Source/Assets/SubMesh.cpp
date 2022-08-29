#include <Core/EnPch.hpp>
#include "SubMesh.hpp"

namespace en
{
	SubMesh::SubMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Material* material)
		: m_Material(material), Asset{ AssetType::SubMesh }
	{
		m_VertexBuffer = std::make_unique<VertexBuffer>(vertices);
		m_IndexBuffer  = std::make_unique<IndexBuffer>(indices);
	}
}