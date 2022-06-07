#pragma once

#ifndef EN_SCENE_HPP
#define EN_SCENE_HPP

#include <Scene/SceneObject.hpp>
#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/Spotlight.hpp>

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

		PointLight* CreatePointLight(glm::vec3 position, glm::vec3 color = glm::vec3(1.0f), float intensity = 2.5f, float radius = 10.0f);
		void DeletePointLight(uint32_t index);

		Spotlight* CreateSpotlight(glm::vec3 position, glm::vec3 direction, glm::vec3 color = glm::vec3(1.0f), float innerCutoff = 0.2f, float outerCutoff = 0.4f, float range = 8.0f, float intensity = 2.5f);
		void DeleteSpotlight(uint32_t index);

		DirectionalLight* CreateDirectionalLight(glm::vec3 direction, glm::vec3 color = glm::vec3(1.0f), float intensity = 2.5f);
		void DeleteDirectionalLight(uint32_t index);

		std::vector<PointLight>&	   GetAllPointLights()		 { return m_PointLights;	   };
		std::vector<Spotlight>&		   GetAllSpotlights()		 { return m_Spotlights;		   };
		std::vector<DirectionalLight>& GetAllDirectionalLights() { return m_DirectionalLights; };

		glm::vec3 m_AmbientColor = glm::vec3(0.0f);

		std::vector<SceneObject*> GetAllSceneObjects();

		const uint32_t& GetSceneObjectCount() const { return m_SceneObjects.size(); };

	private:
		std::unordered_map<std::string, std::unique_ptr<SceneObject>> m_SceneObjects;

		std::vector<PointLight>		  m_PointLights;
		std::vector<Spotlight>		  m_Spotlights;
		std::vector<DirectionalLight> m_DirectionalLights;
	};
}

#endif