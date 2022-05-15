#include <Core/EnPch.hpp>
#include "SceneObject.hpp"

namespace en
{
	SceneObject::SceneObject(Mesh* mesh, std::string name) : m_Mesh(mesh), m_Name(name)
	{
		
	}
	PerObjectData& SceneObject::GetObjectData()
	{
		UpdateModelMatrix();

		return m_Object;
	}
	void SceneObject::UpdateModelMatrix()
	{
		m_Object.model = glm::mat4(1.0f);

		m_Object.model = glm::translate(m_Object.model, m_Position);

		if (m_Rotation != glm::vec3(0.0f))
		{
			m_Object.model = glm::rotate(m_Object.model, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
			m_Object.model = glm::rotate(m_Object.model, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
			m_Object.model = glm::rotate(m_Object.model, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));
		}

		m_Object.model = glm::scale(m_Object.model, m_Scale);
	}
}