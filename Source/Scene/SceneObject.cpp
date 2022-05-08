#include <Core/EnPch.hpp>
#include "SceneObject.hpp"

namespace en
{
	SceneObject::SceneObject(Mesh* mesh) : m_Mesh(mesh)
	{
		
	}
	PerObjectData& SceneObject::GetObjectData()
	{
		UpdateModelMatrix();

		return m_Object;
	}
	void SceneObject::SetMesh(Mesh* newMesh)
	{
		m_Mesh = newMesh;
	}
	const Mesh* SceneObject::GetMesh()
	{
		//This causes unexplained behaviour, the app sometimes crashes on resize sometimes. I have to change the way assets reference each other

		//if (!m_Mesh)
			//m_Mesh = Mesh::GetEmptyMesh();

		return m_Mesh;
	}
	void SceneObject::UpdateModelMatrix()
	{
		m_Object.model = glm::mat4(1.0f);

		m_Object.model = glm::translate(m_Object.model, m_Position);

		m_Object.model = glm::scale(m_Object.model, m_Scale);

		if (m_Rotation != glm::vec3(0.0f))
		{
			m_Object.model = glm::rotate(m_Object.model, glm::radians(m_Rotation.x), glm::vec3(1, 0, 0));
			m_Object.model = glm::rotate(m_Object.model, glm::radians(m_Rotation.y), glm::vec3(0, 1, 0));
			m_Object.model = glm::rotate(m_Object.model, glm::radians(m_Rotation.z), glm::vec3(0, 0, 1));
		}
	}
}