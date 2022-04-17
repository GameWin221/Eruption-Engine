#pragma once

#ifndef EN_MESH_HPP
#define EN_MESH_HPP

#include <tiny_obj_loader.h>

#include <Renderer/Buffers/VertexBuffer.hpp>
#include <Renderer/Buffers/IndexBuffer.hpp>
#include <Renderer/Buffers/UniformBuffer.hpp>

#include <Assets/Material.hpp>

namespace en
{
	class Mesh
	{
	public:
		Mesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);

		Material* m_Material;
	
		std::unique_ptr<VertexBuffer>  m_VertexBuffer;
		std::unique_ptr<IndexBuffer>   m_IndexBuffer;

		static void LoadMesh(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices);
	};
}

#endif