#pragma once

#ifndef EN_SCENE_HPP
#define EN_SCENE_HPP

#include <unordered_map>

#include <Scene/SceneObject.hpp>
#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

namespace en
{
	class Scene
	{
		friend class Renderer;

	public:
		Scene();

		Handle<SceneObject> GetSceneObject(const std::string& name);

		Handle<SceneObject> CreateSceneObject(const std::string& name, Handle <Mesh> mesh);
		void DeleteSceneObject(const std::string& name);

		void RenameSceneObject(const std::string& oldName, const std::string& newName);

		PointLight* CreatePointLight(const glm::vec3 position, const glm::vec3 color = glm::vec3(1.0f), const float intensity = 2.5f, const float radius = 10.0f, const bool active = true);
		void DeletePointLight(const uint32_t index);

		SpotLight* CreateSpotLight(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color = glm::vec3(1.0f), const float innerCutoff = 0.2f, const float outerCutoff = 0.4f, const float range = 8.0f, const float intensity = 2.5f, const bool active = true);
		void DeleteSpotLight(const uint32_t index);

		DirectionalLight* CreateDirectionalLight(const glm::vec3 direction, const glm::vec3 color = glm::vec3(1.0f), const float intensity = 2.5f, const bool active = true);
		void DeleteDirectionalLight(const uint32_t index);

		void UpdateSceneObjects();

		glm::vec3 m_AmbientColor = glm::vec3(0.0f);

		std::vector<PointLight>		  m_PointLights;
		std::vector<SpotLight>		  m_SpotLights;
		std::vector<DirectionalLight> m_DirectionalLights;

		std::vector<Handle<SceneObject>> GetAllSceneObjects();

		const uint32_t& GetSceneObjectCount() const { return m_SceneObjects.size(); };

	private:
		std::unordered_map<std::string, Handle<SceneObject>> m_SceneObjects;
	};
}

#endif