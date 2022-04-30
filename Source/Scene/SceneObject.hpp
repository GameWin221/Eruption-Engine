#pragma once

#ifndef EN_SCENEOBJECT_HPP
#define EN_SCENEOBJECT_HPP

#include <Assets/Mesh.hpp>

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

	class SceneObject
	{
	public:
		SceneObject(Mesh* mesh);
		
		Mesh* m_Mesh;

		glm::vec3 m_Position = glm::vec3(0.0);
		glm::vec3 m_Rotation = glm::vec3(0.0);
		glm::vec3 m_Scale    = glm::vec3(1.0);

		PerObjectData& GetObjectData();

	private:
		PerObjectData m_Object;

		void UpdateModelMatrix();
	};
}

#endif