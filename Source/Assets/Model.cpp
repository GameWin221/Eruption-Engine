#include <Core/EnPch.hpp>
#include "Model.hpp"

namespace en
{
	Model* g_NoModel;

	Model::Model(std::string objPath)
	{
		m_FilePath = objPath;

		m_UniformBuffer = std::make_unique<UniformBuffer>();

		if (objPath.substr(objPath.length() - 4, 4) == ".obj")
			LoadOBJModel();

		else if (objPath.substr(objPath.length() - 5, 5) == ".gltf")
			LoadGLTFModel();

		else
			throw std::runtime_error("Failed to load \"" + objPath + "\" - Unknown file format! Use .obj or .gltf.\n");
	}

	Model* Model::GetNoModel()
	{
		if (!g_NoModel)
			g_NoModel = new Model("Models/NoModel.obj");

		return g_NoModel;
	}

	void Model::LoadOBJModel()
	{
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_FilePath.c_str()))
		{
			if (m_FilePath == "Models/NoModel.obj")
				throw std::runtime_error("Failed to load NoModel.obj! This is an essential file.");

			std::cout << warn + err;

			shapes.clear();
			shapes.shrink_to_fit();

			materials.clear();
			materials.shrink_to_fit();

			// The NoModel.obj should be embedded within the executable itself
			m_FilePath = "Models/NoModel.obj";
			LoadOBJModel();
			return;
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

			m_Meshes.emplace_back(vertices, indices);
		}
	}
	void Model::LoadGLTFModel()
	{
		throw std::runtime_error("There's no gltf loader yet!");
		/*
		tinygltf::Model model;
		tinygltf::TinyGLTF gltf;
		std::string warn, err;
		
		if (!gltf.LoadBinaryFromFile(&model, &err, &warn, m_FilePath.c_str()))
		{
			std::cout << warn + err;

			m_FilePath = "Models/NoModel.obj";
			LoadOBJModel();
			return;
		}
		*/
	}
}