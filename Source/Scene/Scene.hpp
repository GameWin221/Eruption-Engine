#pragma once

#ifndef EN_SCENE_HPP
#define EN_SCENE_HPP

#include <Scene/SceneObject.hpp>

namespace en
{
	class Scene
	{
		friend class VulkanRendererBackend;

	public:
		SceneObject* GetSceneObject(std::string name);

		SceneObject* CreateSceneObject(std::string name, Mesh* mesh);
		void DeleteSceneObject(std::string name);

		void RenameSceneObject(std::string oldName, std::string newName);


		glm::vec3 m_AmbientColor = glm::vec3(0.04f, 0.04f, 0.075f);


		std::vector<SceneObject*> GetAllSceneObjects();

		const uint32_t& GetSceneObjectCount() const { return m_SceneObjects.size(); };

	private:
		std::unordered_map<std::string, std::unique_ptr<SceneObject>> m_SceneObjects;
	};
}

#endif