#include "SubMesh.hpp"

namespace en
{
	SubMesh::SubMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Handle<Material> material)
		: m_Material(material), Asset{ AssetType::SubMesh }
	{
		m_VertexBuffer = MakeHandle<VertexBuffer>(vertices);
		m_IndexBuffer  = MakeHandle<IndexBuffer>(indices);
	}
}