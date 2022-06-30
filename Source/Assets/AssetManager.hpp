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
		~AssetManager();

		static AssetManager* Get();

		bool LoadMesh	(const std::string& nameID, const std::string& path, const MeshImportProperties&    properties = MeshImportProperties{}   );
		bool LoadTexture(const std::string& nameID, const std::string& path, const TextureImportProperties& properties = TextureImportProperties{});

		void DeleteMesh   (const std::string& nameID);
		void DeleteTexture(const std::string& nameID);

		bool CreateMaterial(const std::string& nameID, const glm::vec3& color = glm::vec3(1.0f), const float& metalnessVal = 0.0f, const float& roughnessVal = 0.75f, const float& normalStrength = 1.0f, Texture* albedoTexture = Texture::GetWhiteSRGBTexture(), Texture* roughnessTexture = Texture::GetWhiteNonSRGBTexture(), Texture* normalTexture = Texture::GetWhiteNonSRGBTexture(), Texture* metalnessTexture = Texture::GetWhiteNonSRGBTexture());
		void DeleteMaterial(const std::string& nameID);

		bool ContainsMesh    (const std::string& nameID) { return m_Meshes   .contains(nameID); };
		bool ContainsTexture (const std::string& nameID) { return m_Textures .contains(nameID); };
		bool ContainsMaterial(const std::string& nameID) { return m_Materials.contains(nameID); };

		void RenameMesh    (const std::string& oldNameID, const std::string& newNameID);
		void RenameTexture (const std::string& oldNameID, const std::string& newNameID);
		void RenameMaterial(const std::string& oldNameID, const std::string& newNameID);

		void UpdateAssets();

		Mesh*     GetMesh    (const std::string& nameID);
		Texture*  GetTexture (const std::string& nameID);
		Material* GetMaterial(const std::string& nameID);

		std::vector<Mesh*>     GetAllMeshes();
		std::vector<Texture* >  GetAllTextures();
		std::vector<Material*> GetAllMaterials();

	private:
		void UpdateMaterials();

		std::unordered_map<std::string, std::unique_ptr<Mesh>>     m_Meshes;
		std::unordered_map<std::string, std::unique_ptr<Texture>>  m_Textures;
		std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;

		std::unique_ptr<Mesh> LoadMeshFromFile(const std::string& filePath, const std::string& name, const MeshImportProperties& importProperties);
		void ProcessNode(aiNode* node, const aiScene* scene, const std::vector<Material*>& materials, Mesh* mesh);
		void GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
		void GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices);
	};
}

#endif