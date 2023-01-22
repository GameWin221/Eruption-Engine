#pragma once

#ifndef EN_SUBMESH_HPP
#define EN_SUBMESH_HPP

#include <Renderer/Buffers/VertexBuffer.hpp>
#include <Renderer/Buffers/IndexBuffer.hpp>

#include "Asset.hpp"

#include <Assets/Material.hpp>

namespace en
{
	class SubMesh : public Asset
	{
	public:
		SubMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Handle<Material> material);

		Handle<Material> m_Material;

		Handle<VertexBuffer> m_VertexBuffer;
		Handle<IndexBuffer>  m_IndexBuffer;

		bool m_Active = true;
	};
}

#endif