#pragma once

#ifndef EN_MODEL_HPP
#define EN_MODEL_HPP

#include <tiny_obj_loader.h>

#include "Mesh.hpp"
#include "UniformBuffer.hpp"

namespace en
{
	class Model
	{
	public:
		Model(std::string objPath);

		std::vector<Mesh> m_Meshes;

		std::unique_ptr<UniformBuffer> m_UniformBuffer;

		static Model* GetNoModel();

	private:
		std::string m_FilePath;

		void LoadModel();
	};
}

#endif