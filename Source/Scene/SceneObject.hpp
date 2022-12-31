#pragma once

#ifndef EN_SCENEOBJECT_HPP
#define EN_SCENEOBJECT_HPP

#include <Assets/Mesh.hpp>
#include <Assets/AssetManager.hpp>

#include <Scene/SceneMember.hpp>

namespace en
{
	class SceneObject : public SceneMember
	{
		friend class Scene;

	public:
		SceneObject(Mesh* mesh, const std::string& name) 
			: m_Mesh(mesh), m_Name(name), SceneMember{ SceneMemberType::SceneObject }{};

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Rotation = glm::vec3(0.0);
		glm::vec3 m_Scale    = glm::vec3(1.0);

		void UpdateModelMatrix();

		const glm::mat4& GetModelMatrix() const { return m_ModelMatrix; };

		Mesh* m_Mesh;

		bool m_Active = true;

		const std::string& GetName() const { return m_Name; };

	private:
		glm::mat4 m_ModelMatrix;

		std::string m_Name;
	};
}

#endif