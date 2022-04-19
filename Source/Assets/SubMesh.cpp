#include <Core/EnPch.hpp>
#include "SubMesh.hpp"

namespace en
{
	SubMesh::SubMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, Material* material) : m_Material(material)
	{
		m_VertexBuffer = std::make_unique<VertexBuffer>(vertices);
		m_IndexBuffer  = std::make_unique<IndexBuffer>(indices);
	}
}