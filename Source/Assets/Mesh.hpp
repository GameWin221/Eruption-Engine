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
	public:
		std::vector<SubMesh> m_SubMeshes;

		std::string m_FilePath;
	};
}

#endif