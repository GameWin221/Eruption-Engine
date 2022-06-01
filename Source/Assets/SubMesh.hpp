#pragma once

#ifndef EN_SUBMESH_HPP
#define EN_SUBMESH_HPP

#include <Renderer/Buffers/VertexBuffer.hpp>
#include <Renderer/Buffers/IndexBuffer.hpp>

#include <Assets/Material.hpp>

namespace en
{
	class SubMesh
	{
	public:
		SubMesh(std::vector<Vertex>& vertices, std::vector<uint32_t>& indices, Material* material);

		void Draw(VkCommandBuffer& cmd);

		Material* m_Material;

		std::unique_ptr<VertexBuffer> m_VertexBuffer;
		std::unique_ptr<IndexBuffer>  m_IndexBuffer;

		const glm::vec3& GetCenter() const { return m_Center; }

		bool m_Active = true;

	private:
		glm::vec3 m_Center;
	};
}

#endif