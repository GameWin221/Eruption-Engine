#pragma once

#ifndef EN_SUBMESH_HPP
#define EN_SUBMESH_HPP

#include <tiny_obj_loader.h>

#include <Renderer/Buffers/VertexBuffer.hpp>
#include <Renderer/Buffers/IndexBuffer.hpp>
#include <Renderer/Buffers/UniformBuffer.hpp>

#include <Assets/Material.hpp>

namespace en
{
	class SubMesh
	{
	public:
		SubMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, Material* material);
	
		Material* m_Material;

		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer>  m_IndexBuffer;
	};
}

#endif