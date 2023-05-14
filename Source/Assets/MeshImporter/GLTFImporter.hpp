#pragma once
#ifndef EN_GLTFIMPORTER_HPP
#define EN_GLTFIMPORTER_HPP

#include "Importer.hpp"
#include <json.hpp>
#include <fstream>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <gtc/type_ptr.hpp>
#include <glm.hpp>


namespace en
{
	class GLTFImporter : public Importer
	{
	public:
		GLTFImporter(const MeshImportProperties& properties, Handle<Material> defaultMaterial, Handle<Texture> defaultSRGBTexture, Handle<Texture> defaultNonSRGBTexture);

		MeshData LoadMeshFromFile(const std::string& filePath, const std::string& name);

	private:
		nlohmann::json JSON{};

		std::string m_FilePath{};
		std::string m_FileDirectory{};

		std::vector<char> m_BinaryData{};

		std::vector<Handle<Material>> m_Materials;
		std::vector<Handle<Texture>> m_Textures;

	private:
		void ProcessNode(uint32_t id, Handle<Mesh> mesh);
		void ProcessMesh(uint32_t id, Handle<Mesh> mesh);

		std::vector<float> GetFloats(const nlohmann::json& accessor);
		std::vector<uint32_t> GetIndices(const nlohmann::json& accessor);

		void GetMaterials();

		nlohmann::json GetMeshJsonData();
		std::vector<char> GetMeshBinaryData();
	};
}


#endif // !EN_GLTFIMPORTER_HPP