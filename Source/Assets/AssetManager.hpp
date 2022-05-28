#pragma once

#ifndef EN_ASSETMANAGER_HPP
#define EN_ASSETMANAGER_HPP

#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>

namespace en
{
	enum struct TextureFormat
	{
		Color    = VK_FORMAT_R8G8B8A8_SRGB,
		NonColor = VK_FORMAT_R8G8B8A8_UNORM
	};
	struct MeshImportProperties
	{
		bool importMaterials = true;

		bool importAlbedoTextures	  = true;
		bool importRoughnessTextures  = true;
		bool importMetalnessTextures  = true;
		bool importNormalTextures     = true;

		bool importColor = true;
	};
	struct TextureImportProperties
	{
		TextureFormat format = TextureFormat::Color;

		bool flipped = false;
	};

	class AssetManager
	{
	public:
		AssetManager();

		static AssetManager* Instance();

		bool LoadMesh	(std::string nameID, std::string path, MeshImportProperties properties    = MeshImportProperties{}   );
		bool LoadTexture(std::string nameID, std::string path, TextureImportProperties properties = TextureImportProperties{});

		void DeleteMesh   (std::string nameID);
		void DeleteTexture(std::string nameID);

		bool CreateMaterial(std::string nameID, glm::vec3 color = glm::vec3(1.0f), float metalnessVal = 0.0f, float roughnessVal = 0.75f, float normalStrength = 1.0f, Texture* albedoTexture = Texture::GetWhiteSRGBTexture(), Texture* roughnessTexture = Texture::GetWhiteNonSRGBTexture(), Texture* normalTexture = Texture::GetWhiteNonSRGBTexture(), Texture* metalnessTexture = Texture::GetWhiteNonSRGBTexture());
		void DeleteMaterial(std::string nameID);

		bool ContainsMesh    (std::string nameID) { return m_Meshes   .contains(nameID); };
		bool ContainsTexture (std::string nameID) { return m_Textures .contains(nameID); };
		bool ContainsMaterial(std::string nameID) { return m_Materials.contains(nameID); };

		void RenameMesh    (std::string oldNameID, std::string newNameID);
		void RenameTexture (std::string oldNameID, std::string newNameID);
		void RenameMaterial(std::string oldNameID, std::string newNameID);

		void UpdateAssets();

		Mesh*     GetMesh    (std::string nameID);
		Texture*  GetTexture (std::string nameID);
		Material* GetMaterial(std::string nameID);

		std::vector<Mesh*>     GetAllMeshes();
		std::vector<Texture* >  GetAllTextures();
		std::vector<Material*> GetAllMaterials();

	private:
		void UpdateMaterials();


		std::unordered_map<std::string, std::unique_ptr<Mesh>>     m_Meshes;
		std::unordered_map<std::string, std::unique_ptr<Texture>>  m_Textures;
		std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;

		std::unique_ptr<Mesh> LoadMeshFromFile(std::string& filePath, std::string& name, MeshImportProperties& importProperties);
		void ProcessNode(aiNode* node, const aiScene* scene, std::vector<Material*> materials, Mesh* mesh);
		void GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
		void GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices);
	};
}

#endif