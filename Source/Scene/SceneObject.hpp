#pragma once

#ifndef EN_SCENEOBJECT_HPP
#define EN_SCENEOBJECT_HPP

#include <Assets/Mesh.hpp>
#include <Assets/AssetManager.hpp>

#include <Scene/SceneMember.hpp>

#include <glm.hpp>
#include <gtx/transform.hpp>

namespace en
{
	class SceneObject : public SceneMember
	{
		friend class Scene;

	public:
		SceneObject(Handle<Mesh> mesh, const std::string& name)
			: m_Mesh(mesh), m_Name(name), SceneMember{ SceneMemberType::SceneObject }{};

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Rotation = glm::vec3(0.0);
		glm::vec3 m_Scale    = glm::vec3(1.0);

		Handle<Mesh> m_Mesh;

		bool m_Active = true;

		const glm::mat4& GetModelMatrix() const { return m_ModelMatrix; };

		const std::string& GetName() const { return m_Name; };

	private:
		void UpdateModelMatrix();

		glm::mat4 m_ModelMatrix = glm::mat4(1.0);

		std::string m_Name;
	};
}

#endif