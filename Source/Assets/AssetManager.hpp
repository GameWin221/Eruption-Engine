#pragma once

#ifndef EN_ASSETMANAGER_HPP
#define EN_ASSETMANAGER_HPP

#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>

#include <unordered_map>
#include <string>

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

		static AssetManager& Get();

		bool LoadMesh	(const std::string& nameID, const std::string& path, const MeshImportProperties&    properties = MeshImportProperties{}   );
		bool LoadTexture(const std::string& nameID, const std::string& path, const TextureImportProperties& properties = TextureImportProperties{});

		void DeleteMesh   (const std::string& nameID);
		void DeleteTexture(const std::string& nameID);

		bool CreateMaterial(const std::string& nameID, const glm::vec3 color = glm::vec3(1.0f), const float metalnessVal = 0.0f, const float roughnessVal = 0.75f, const float normalStrength = 1.0f, Handle<Texture> albedoTexture = nullptr, Handle<Texture> roughnessTexture = nullptr, Handle<Texture> normalTexture = nullptr, Handle<Texture> metalnessTexture = nullptr);
		void DeleteMaterial(const std::string& nameID);

		bool ContainsMesh    (const std::string& nameID) { return m_Meshes   .contains(nameID); };
		bool ContainsTexture (const std::string& nameID) { return m_Textures .contains(nameID); };
		bool ContainsMaterial(const std::string& nameID) { return m_Materials.contains(nameID); };

		void RenameMesh    (const std::string& oldNameID, const std::string& newNameID);
		void RenameTexture (const std::string& oldNameID, const std::string& newNameID);
		void RenameMaterial(const std::string& oldNameID, const std::string& newNameID);

		Handle<Mesh>     GetMesh    (const std::string& nameID);
		Handle<Texture>  GetTexture (const std::string& nameID);
		Handle<Material> GetMaterial(const std::string& nameID);

		Handle<Texture> GetWhiteSRGBTexture();
		Handle<Texture> GetWhiteNonSRGBTexture();

		Handle<Material> GetDefaultMaterial();

		std::vector<Handle<Mesh>>     GetAllMeshes();
		std::vector<Handle<Texture>>  GetAllTextures();
		std::vector<Handle<Material>> GetAllMaterials();

	private:
		void UpdateMaterials();

		std::unordered_map<std::string, Handle<Mesh>    > m_Meshes;
		std::unordered_map<std::string, Handle<Texture> > m_Textures;
		std::unordered_map<std::string, Handle<Material>> m_Materials;

		Handle<Texture> m_SRGBWhiteTexture;
		Handle<Texture> m_NSRGBTexture;

		Handle<Material> m_DefaultMaterial;

		uint32_t m_MatIndex = 0U;

		Handle<Mesh> LoadMeshFromFile(const std::string& filePath, const std::string& name, const MeshImportProperties& importProperties);
		void ProcessNode(aiNode* node, const aiScene* scene, const std::vector<Handle<Material>>& materials, Handle<Mesh> mesh);
		void GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices);
		void GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices);
	};
}

#endif