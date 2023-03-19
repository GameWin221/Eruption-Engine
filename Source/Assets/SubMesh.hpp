#pragma once

#ifndef EN_SUBMESH_HPP
#define EN_SUBMESH_HPP

#include <Renderer/Buffers/MemoryBuffer.hpp>
#include <Renderer/Buffers/Vertex.hpp>

#include "Asset.hpp"

#include <Assets/Material.hpp>

namespace en
{
	class SubMesh : public Asset
	{
		friend class Scene;

	public:
		SubMesh(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Handle<Material> material);

		Handle<MemoryBuffer> m_VertexBuffer;
		Handle<MemoryBuffer> m_IndexBuffer;
		
		Handle<Material> m_Material;

		const uint32_t m_VertexCount;
		const uint32_t m_IndexCount;

		bool m_Active = true;

		const uint32_t& GetMaterialIndex() const { return m_MaterialIndex; };

	private:
		uint32_t m_MaterialIndex{};
	};
}

#endif