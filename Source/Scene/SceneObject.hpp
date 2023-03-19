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

		Handle<Mesh> m_Mesh;

		bool m_Active = true;

		void SetPosition(const glm::vec3& position);
		void SetRotation(const glm::vec3& rotation);
		void SetScale	(const glm::vec3& scale);

		const glm::vec3& GetPosition() const { return m_Position; };
		const glm::vec3& GetRotation() const { return m_Rotation; };
		const glm::vec3& GetScale()	   const { return m_Scale;    };

		const uint32_t& GetMatrixIndex() const { return m_MatrixIndex; };

		const std::string& GetName() const { return m_Name; };

	private:
		glm::vec3 m_Position = glm::vec3(0.0f);
		glm::vec3 m_Rotation = glm::vec3(0.0f);
		glm::vec3 m_Scale	 = glm::vec3(1.0f);

		bool m_TransformChanged = true;

		uint32_t m_MatrixIndex{};

		std::string m_Name;
	};
}

#endif