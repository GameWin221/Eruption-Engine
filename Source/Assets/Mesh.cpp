#include <Core/EnPch.hpp>
#include "Mesh.hpp"

namespace en
{
	Mesh::Mesh(Texture* texture, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		m_Texture		= texture;
		m_VertexBuffer  = std::make_unique<VertexBuffer>(vertices);
		m_IndexBuffer   = std::make_unique<IndexBuffer>(indices);
	}

	void Mesh::LoadMesh(std::string path, std::vector<Vertex>& vertices, std::vector<uint32_t>& indices)
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, path.c_str()))
		{
			std::cout << warn + err;

			shapes.clear();
			shapes.shrink_to_fit();

			materials.clear();
			materials.shrink_to_fit();

			tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "Models/NoModel.obj");
		}

		
		if (attrib.normals.size() == 0)
			attrib.normals.resize(attrib.vertices.size());
		
		if (attrib.texcoords.size() == 0)
			attrib.texcoords.resize((attrib.vertices.size() / 3) * 2);

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};

		Vertex vertex{};
		for (const auto& shape : shapes) 
		{
			for (const auto& index : shape.mesh.indices) 
			{
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

				vertex.texcoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1]
				};

				if (!uniqueVertices.contains(vertex)) 
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.emplace_back(vertex);
				}

				indices.emplace_back(uniqueVertices[vertex]);
			}
		}
	}
}