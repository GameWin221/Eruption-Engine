#pragma once

#ifndef EN_SCENEOBJECT_HPP
#define EN_SCENEOBJECT_HPP

#include <Assets/Mesh.hpp>

namespace en
{
	class SceneObject
	{
	public:
		SceneObject(Mesh* mesh);
		
		Mesh* m_Mesh;

		std::unique_ptr<UniformBuffer> m_UniformBuffer;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Rotation = glm::vec3(0.0);
		glm::vec3 m_Scale    = glm::vec3(1.0);

		const glm::mat4& GetModelMatrix();

	private:
		glm::mat4 m_ModelMatrix = glm::mat4(1.0f);
	};
}

#endif