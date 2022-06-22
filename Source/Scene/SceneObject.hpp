#pragma once

#ifndef EN_SCENEOBJECT_HPP
#define EN_SCENEOBJECT_HPP

#include <Assets/Mesh.hpp>
#include <Assets/AssetManager.hpp>

#include <Scene/SceneMember.hpp>

namespace en
{
	struct PerObjectData
	{
		glm::mat4 model = glm::mat4(1.0f);

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout)
		{
			vkCmdPushConstants(cmd, layout, VK_SHADER_STAGE_VERTEX_BIT, 0U, sizeof(PerObjectData), this);
		}
	};

	class SceneObject : public SceneMember
	{
		friend class Scene;

	public:
		SceneObject(Mesh* mesh, std::string name) : m_Mesh(mesh), m_Name(name), SceneMember{ SceneMemberType::SceneObject }{};

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Rotation = glm::vec3(0.0);
		glm::vec3 m_Scale    = glm::vec3(1.0);

		PerObjectData& GetObjectData();

		Mesh* m_Mesh;

		bool m_Active = true;

		// Renaming is in the Scene class

		const std::string& GetName() const { return m_Name; };

	private:
		std::string m_Name;

		PerObjectData m_Object;

		void UpdateModelMatrix();
	};
}

#endif