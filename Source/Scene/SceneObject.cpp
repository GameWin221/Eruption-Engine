#include "Core/EnPch.hpp"
#include "SceneObject.hpp"

namespace en
{
	SceneObject::SceneObject(Mesh* mesh) : m_Mesh(mesh)
	{
		m_UniformBuffer = std::make_unique<UniformBuffer>();
	}
	const glm::mat4& SceneObject::GetModelMatrix()
	{
		m_ModelMatrix = glm::mat4(1.0f);

		m_ModelMatrix = glm::translate(m_ModelMatrix, m_Position);

		if (m_Rotation != glm::vec3(0.0f, 0.0f, 0.0f))
		{
			m_ModelMatrix = glm::rotate(m_ModelMatrix, glm::radians(45.0f), m_Rotation/180.0f);
			std::cout << "Rotation: (x:" << m_Rotation.x << ", y:" << m_Rotation.y << ", z:" << m_Rotation.z << ")\n";
		}

		m_ModelMatrix = glm::scale(m_ModelMatrix, m_Scale);

		return m_ModelMatrix;
	}
}