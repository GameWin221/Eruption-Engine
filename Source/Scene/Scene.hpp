#pragma once

#ifndef EN_SCENE_HPP
#define EN_SCENE_HPP

#include <Scene/SceneObject.hpp>
#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

namespace en
{
	class Scene
	{
		friend class RendererBackend;

	public:
		SceneObject* GetSceneObject(std::string name);

		SceneObject* CreateSceneObject(std::string name, Mesh* mesh);
		void DeleteSceneObject(std::string name);

		void RenameSceneObject(std::string oldName, std::string newName);

		PointLight* CreatePointLight(glm::vec3 position, glm::vec3 color = glm::vec3(1.0f), float intensity = 2.5f, float radius = 10.0f, bool active = true);
		void DeletePointLight(uint32_t index);

		SpotLight* CreateSpotLight(glm::vec3 position, glm::vec3 direction, glm::vec3 color = glm::vec3(1.0f), float innerCutoff = 0.2f, float outerCutoff = 0.4f, float range = 8.0f, float intensity = 2.5f, bool active = true);
		void DeleteSpotLight(uint32_t index);

		DirectionalLight* CreateDirectionalLight(glm::vec3 direction, glm::vec3 color = glm::vec3(1.0f), float intensity = 2.5f, bool active = true);
		void DeleteDirectionalLight(uint32_t index);

		glm::vec3 m_AmbientColor = glm::vec3(0.0f);

		std::vector<PointLight>		  m_PointLights;
		std::vector<SpotLight>		  m_SpotLights;
		std::vector<DirectionalLight> m_DirectionalLights;

		std::vector<SceneObject*> GetAllSceneObjects();

		const uint32_t& GetSceneObjectCount() const { return m_SceneObjects.size(); };

	private:
		std::unordered_map<std::string, std::unique_ptr<SceneObject>> m_SceneObjects;
	};
}

#endif