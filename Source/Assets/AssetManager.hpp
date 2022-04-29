#pragma once

#ifndef EN_ASSETMANAGER_HPP
#define EN_ASSETMANAGER_HPP

#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>

namespace en
{
	class AssetManager
	{
	public:
		AssetManager();

		void LoadMesh	(std::string nameID, std::string path);
		void LoadTexture(std::string nameID, std::string path, VkFormat format = VK_FORMAT_R8G8B8A8_SRGB, bool loadFlipped = true);

		void UnloadMesh   (std::string nameID);
		void UnloadTexture(std::string nameID);

		void CreateMaterial(std::string nameID, glm::vec3 color = glm::vec3(1.0f), float shininess = 32.0f, float normalStrength = 0.5f, Texture* albedoTexture = nullptr, Texture* specularTexture = nullptr, Texture* normalTexture = nullptr);
		void DeleteMaterial(std::string nameID);

		Mesh*     GetMesh    (std::string nameID);
		Texture*  GetTexture (std::string nameID);
		Material* GetMaterial(std::string nameID);

	private:
		std::unordered_map<std::string, std::unique_ptr<Mesh    >> m_Meshes;
		std::unordered_map<std::string, std::unique_ptr<Texture >> m_Textures;
		std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	};
}


#endif