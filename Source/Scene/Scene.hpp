#pragma once

#ifndef EN_SCENE_HPP
#define EN_SCENE_HPP

#include <unordered_map>
#include <unordered_set>

#include <Scene/SceneObject.hpp>
#include <Renderer/Lights/PointLight.hpp>
#include <Renderer/Lights/DirectionalLight.hpp>
#include <Renderer/Lights/SpotLight.hpp>

#include <Renderer/Camera/Camera.hpp>

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

		//void RenameSceneObject(const std::string& oldName, const std::string& newName);

		PointLight* CreatePointLight(const glm::vec3 position, const glm::vec3 color = glm::vec3(1.0f), const float intensity = 2.5f, const float radius = 10.0f, const bool active = true);
		void DeletePointLight(const uint32_t index);

		SpotLight* CreateSpotLight(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color = glm::vec3(1.0f), const float innerCutoff = 0.2f, const float outerCutoff = 0.4f, const float range = 8.0f, const float intensity = 2.5f, const bool active = true);
		void DeleteSpotLight(const uint32_t index);

		DirectionalLight* CreateDirectionalLight(const glm::vec3 direction, const glm::vec3 color = glm::vec3(1.0f), const float intensity = 2.5f, const bool active = true);
		void DeleteDirectionalLight(const uint32_t index);

		static VkDescriptorSetLayout GetGlobalDescriptorLayout();

		glm::vec3 m_AmbientColor = glm::vec3(0.0f);

		std::vector<PointLight>		  m_PointLights;
		std::vector<SpotLight>		  m_SpotLights;
		std::vector<DirectionalLight> m_DirectionalLights;

		Handle<Camera> m_MainCamera;

		Handle<DescriptorSet> m_GlobalDescriptorSet;

		std::unordered_map<std::string, Handle<SceneObject>> m_SceneObjects;

	private:
		struct GPUMaterial {
			glm::vec3 color = glm::vec3(1.0f);
			float _padding0;
			
			float metalnessVal   = 0.00f;
			float roughnessVal   = 0.75f;
			float normalStrength = 1.00f;
			float _padding1;

			uint32_t albedoId{};
			uint32_t roughnessId{};
			uint32_t metalnessId{};
			uint32_t normalId{};
		};

		void UpdateSceneCPU();
		void UpdateSceneGPU();

		uint32_t RegisterMatrix(const glm::mat4& matrix = glm::mat4(1.0f));
		uint32_t RegisterMaterial(Handle<Material> material);
		uint32_t RegisterTexture(Handle<Texture> texture);
		
		void DeregisterMatrix(uint32_t index);
		void DeregisterMaterial(uint32_t index);
		void DeregisterTexture(uint32_t index);

		void UpdateMatrixBuffer	   (const std::vector<uint32_t>& changedMatrixIds);
		void UpdateMaterialBuffer  (const std::vector<uint32_t>& changedMaterialIds);
		void UpdateGlobalDescriptor();
		void UpdateLightsBuffer    (const std::vector<uint32_t>& changedPointLightsIDs, const std::vector<uint32_t>& changedSpotLightsIDs, const std::vector<uint32_t>& changedDirLightsIDs);

		struct GPULights {
			std::array<PointLight::Buffer	   , MAX_POINT_LIGHTS> pointLights{};
			std::array<SpotLight::Buffer	   , MAX_SPOT_LIGHTS> spotLights{};
			std::array<DirectionalLight::Buffer, MAX_DIR_LIGHTS> dirLights{};

			uint32_t activePointLights = 0U;
			uint32_t activeSpotLights = 0U;
			uint32_t activeDirLights = 0U;
			uint32_t _padding0{};

			glm::vec3 ambientLight = glm::vec3(0.0f);
			uint32_t _padding1{};
		} m_GPULights;

		std::vector<glm::mat4  > m_Matrices;
		std::vector<GPUMaterial> m_GPUMaterials;

		std::vector<Handle<Material>> m_Materials;
		std::vector<Handle<Texture> > m_Textures;

		std::unordered_set<uint32_t> m_OccupiedMatrices;
		std::unordered_set<uint32_t> m_OccupiedMaterials;
		std::unordered_set<uint32_t> m_OccupiedTextures;

		std::unordered_map<std::string, uint32_t> m_RegisteredTextures;
		std::unordered_map<std::string, uint32_t> m_RegisteredMaterials;

		Handle<MemoryBuffer> m_LightsBuffer;
		Handle<MemoryBuffer> m_LightsStagingBuffer;

		Handle<MemoryBuffer> m_GlobalMaterialsBuffer;
		Handle<MemoryBuffer> m_GlobalMaterialsStagingBuffer;

		Handle<MemoryBuffer> m_GlobalMatricesBuffer;
		Handle<MemoryBuffer> m_GlobalMatricesStagingBuffer;

		std::vector<uint32_t> m_ChangedMatrixIDs;
		std::vector<uint32_t> m_ChangedMaterialIDs;

		std::vector<uint32_t> m_ChangedPointLightsIDs;
		std::vector<uint32_t> m_ChangedSpotLightsIDs;
		std::vector<uint32_t> m_ChangedDirLightsIDs;

		bool m_SceneLightingChanged = true;

		bool m_GlobalDescriptorChanged = true;
	};
}

#endif