#pragma once

#ifndef EN_MESH_HPP
#define EN_MESH_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/pbrmaterial.h>

#include "Asset.hpp"

#include <Assets/SubMesh.hpp>

namespace en
{
	class Mesh : public Asset
	{
		friend class AssetManager;

	public:
		Mesh() : Asset{ AssetType::Mesh } {};

		std::vector<SubMesh> m_SubMeshes;

		const std::string& GetName()	 const { return m_Name;     };
		const std::string& GetFilePath() const { return m_FilePath; };

		static Handle<Mesh> GetEmptyMesh();

		bool m_Active = true;

	private:
		std::string m_Name;
		std::string m_FilePath;
	};
}

#endif