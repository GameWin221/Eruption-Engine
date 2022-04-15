#include "EnPch.hpp"
#include "Rendering/Model.hpp"

namespace en
{
	Model* g_NoModel;

	Model::Model(std::string objPath)
	{
		m_FilePath = objPath;

		m_UniformBuffer = std::make_unique<UniformBuffer>();

		LoadModel();
	}

	Model* Model::GetNoModel()
	{
		if (!g_NoModel)
			g_NoModel = new Model("Models/NoModel.obj");

		return g_NoModel;
	}

	void Model::LoadModel()
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_FilePath.c_str()))
		{
			std::cout << warn + err;

			shapes.clear();
			shapes.shrink_to_fit();

			materials.clear();
			materials.shrink_to_fit();

			tinyobj::LoadObj(&attrib, &shapes, &materials, &err, "Models/NoModel.obj");
		}

		Vertex vertex{};
		for (const auto& shape : shapes)
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			std::unordered_map<Vertex, uint32_t> uniqueVertices{};

			for (const auto& index : shape.mesh.indices)
			{
				vertex.pos = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};

				if (attrib.normals.size() > 0)
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
				else
					vertex.normal = {1.0f, 1.0f, 1.0f };

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

			m_Meshes.emplace_back(Texture::GetWhiteTexture(), vertices, indices);
		}
	}
}