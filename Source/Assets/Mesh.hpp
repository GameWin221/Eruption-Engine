#pragma once

#ifndef EN_MESH_HPP
#define EN_MESH_HPP

#include <tiny_obj_loader.h>
//#include <tiny_gltf.h>

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

		void LoadOBJMesh();
		void LoadGLTFMesh();
	};
}

#endif