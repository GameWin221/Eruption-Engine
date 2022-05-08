#pragma once

#ifndef EN_MESH_HPP
#define EN_MESH_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Assets/SubMesh.hpp>

namespace en
{
	class Mesh
	{
		friend class AssetManager;

	public:
		std::vector<SubMesh> m_SubMeshes;

		const std::string& GetName()	 const { return m_Name;     };
		const std::string& GetFilePath() const { return m_FilePath; };

		static Mesh* GetEmptyMesh();

		bool m_Active = true;

	private:
		std::string m_Name;
		std::string m_FilePath;
	};
}

#endif