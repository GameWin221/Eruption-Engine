#pragma once

#ifndef EN_MESH_HPP
#define EN_MESH_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Renderer/Buffers/UniformBuffer.hpp>

#include <Assets/SubMesh.hpp>

namespace en
{
	class Mesh
	{
	public:
		Mesh(std::string objPath);

		std::vector<SubMesh> m_SubMeshes;

		static Mesh* GetDefaultMesh();

	private:
		std::string m_FilePath;

		void LoadMesh();
		void ProcessNode(aiNode* node, const aiScene* scene);
		void GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices, const aiScene* scene);
		void GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices, const aiScene* scene);
		//void GetMaterials(aiMesh* mesh, std::vector<SubMesh>& submeshes, const aiScene* scene);
	};
}

#endif