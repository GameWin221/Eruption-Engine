#pragma once

#ifndef EN_ASSETMANAGER_HPP
#define EN_ASSETMANAGER_HPP

#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>

namespace en
{
	enum struct TextureFormat
	{
		Color = VK_FORMAT_R8G8B8A8_SRGB,
		NonColor = VK_FORMAT_R8G8B8A8_UNORM
	};
	struct MeshImportProperties
	{
		bool importMaterials = true;
		bool overwrite = false;
	};
	struct TextureImportProperties
	{
		TextureFormat format = TextureFormat::Color;
		bool overwrite = false;
		bool flipped = false;
	};
	
	const MeshImportProperties    g_DefaultMeshImportSettings;
	const TextureImportProperties g_DefaultTextureImportSettings;

	class AssetManager
	{
	public:
		AssetManager();

		bool LoadMesh	(std::string nameID, std::string path, MeshImportProperties properties    = g_DefaultMeshImportSettings   );
		bool LoadTexture(std::string nameID, std::string path, TextureImportProperties properties = g_DefaultTextureImportSettings);

		void UnloadMesh   (std::string nameID);
		void UnloadTexture(std::string nameID);

		bool CreateMaterial(std::string nameID, glm::vec3 color = glm::vec3(1.0f), float shininess = 48.0f, float normalStrength = 0.5f, Texture* albedoTexture = nullptr, Texture* specularTexture = nullptr, Texture* normalTexture = nullptr);
		void DeleteMaterial(std::string nameID);

		bool ContainsMesh    (std::string nameID) { return m_Meshes   .contains(nameID); };
		bool ContainsTexture (std::string nameID) { return m_Textures .contains(nameID); };
		bool ContainsMaterial(std::string nameID) { return m_Materials.contains(nameID); };

		Mesh*     GetMesh    (std::string nameID);
		Texture*  GetTexture (std::string nameID);
		Material* GetMaterial(std::string nameID);

		std::vector<Mesh*>     GetAllMeshes();
		std::vector<Texture*>  GetAllTextures();
		std::vector<Material*> GetAllMaterials();

	private:
		std::unordered_map<std::string, std::unique_ptr<Mesh    >> m_Meshes;
		std::unordered_map<std::string, std::unique_ptr<Texture >> m_Textures;
		std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;

		std::unique_ptr<Mesh> LoadMeshFromFile(std::string& filePath, bool& importMaterial);
		void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Material*> materials, Mesh* mesh);
		void GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
		void GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices);
	};
}


#endif