#include <Core/EnPch.hpp>
#include "Mesh.hpp"

namespace en
{
	Mesh* g_DefaultMesh;


	Mesh::Mesh(std::string objPath)
	{
		m_FilePath = objPath;

		LoadMesh();
	}

	Mesh* Mesh::GetDefaultMesh()
	{
		if (!g_DefaultMesh)
			g_DefaultMesh = new Mesh("Models/NoModel.obj");

		return g_DefaultMesh;
	}

	void Mesh::LoadMesh()
	{
		Assimp::Importer importer;
		const aiScene* loadedScene = importer.ReadFile(m_FilePath, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
		
		if (!loadedScene || loadedScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !loadedScene->mRootNode)
			EN_ERROR("Mesh::LoadMesh() - " + std::string(importer.GetErrorString()));

		std::string directory = m_FilePath.substr(0, m_FilePath.find_last_of('/'));

		ProcessNode(loadedScene->mRootNode, loadedScene);
	}
	void Mesh::ProcessNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];

			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			GetVertices(mesh, vertices, scene);
			GetIndices(mesh, indices, scene);

			m_SubMeshes.emplace_back(vertices, indices, Material::GetDefaultMaterial());
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
			ProcessNode(node->mChildren[i], scene);
		
	}
	void Mesh::GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices, const aiScene* scene)
	{
		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
			vertices.emplace_back(
				glm::vec3(mesh->mVertices[i].x        , mesh->mVertices[i].y , mesh->mVertices[i].z),
				glm::vec3(mesh->mNormals[i].x         , mesh->mNormals[i].y  , mesh->mNormals[i].z ),
				glm::vec3(mesh->mTangents[i].x        , mesh->mTangents[i].y , mesh->mTangents[i].z),
				glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
			);
	}
	void Mesh::GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices, const aiScene* scene)
	{
		for (unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];

			for (unsigned int j = 0; j < face.mNumIndices; j++)
				indices.emplace_back(face.mIndices[j]);
		}
	}
	//void Mesh::GetMaterials(aiMesh* mesh, std::vector<SubMesh>& submeshes, const aiScene* scene)
	//{
		/*
		if (mesh->mMaterialIndex >= 0)
		{
			aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

			vector<Texture> diffuseMaps = loadMaterialTextures(material,
				aiTextureType_DIFFUSE, "texture_diffuse");
			textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
			vector<Texture> specularMaps = loadMaterialTextures(material,
				aiTextureType_SPECULAR, "texture_specular");
			textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		}
		*/
	//}
	/*
	void Mesh::LoadOBJMesh()
	{
		
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &err, m_FilePath.c_str()))
		{
			EN_WARN("Model::LoadOBJMesh() - Failed to load an .obj mesh from " + m_FilePath + "\"!");

			if (m_FilePath == "Models/NoModel.obj")
				EN_ERROR("Model::LoadOBJMesh() - Failed to load NoModel.obj! This is an essential file.");

			shapes.clear();
			shapes.shrink_to_fit();

			materials.clear();
			materials.shrink_to_fit();

			// The NoModel.obj should be embedded within the executable itself
			m_FilePath = "Models/NoModel.obj";
			LoadOBJMesh();
			return;
		}

		Vertex vertex{};
		Vertex* lastVertex = nullptr;
		Vertex* lastLastVertex = nullptr;;

		for (const auto& shape : shapes)
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;

			std::unordered_map<Vertex, uint32_t> uniqueVertices{};

			for (int v = 0, i = 0; const auto& index : shape.mesh.indices)
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

				if (v >= 2)
				{
					Vertex& v0 = vertices[i - 2];
					Vertex& v1 = vertices[i - 1];
					Vertex& v2 = vertex;

					glm::vec3 tangent = glm::normalize(CalculateTangent(v0, v1, v2));

					v0.tangent = tangent;
					v1.tangent = tangent;
					v2.tangent = tangent;

					v = 0;
				}
				else
					v++;

				i++;

				if (!uniqueVertices.contains(vertex))
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
					vertices.emplace_back(vertex);
				}

				indices.emplace_back(uniqueVertices[vertex]);

				lastLastVertex = lastVertex;
				lastVertex = &uniqueVertices.at(vertex).first;
			}

			m_SubMeshes.emplace_back(vertices, indices, Material::GetDefaultMaterial());
		}

		EN_SUCCESS("Successfully loaded a mesh from \"" + m_FilePath + "\"");
	}
	void Mesh::LoadGLTFMesh()
	{
		EN_ERROR("There's no gltf loader yet!");
		
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
		
		//EN_SUCCESS("Loaded a mesh from \"" + m_FilePath + "\"");
	}
	*/
}