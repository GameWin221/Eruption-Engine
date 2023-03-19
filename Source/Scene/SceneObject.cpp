#include "SceneObject.hpp"

namespace en
{
	void SceneObject::UpdateModelMatrix()
	{
		m_ModelMatrix = glm::translate(glm::mat4(1.0f), m_Position);

		if (m_Rotation != glm::vec3(0.0f))
		{
			m_ModelMatrix = glm::rotate(m_ModelMatrix, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
			m_ModelMatrix = glm::rotate(m_ModelMatrix, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));
			m_ModelMatrix = glm::rotate(m_ModelMatrix, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
		}

		m_ModelMatrix = glm::scale(m_ModelMatrix, m_Scale);
	}
}