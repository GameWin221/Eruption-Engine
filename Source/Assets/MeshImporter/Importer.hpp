#pragma once
#ifndef EN_IMPORTER_HPP
#define EN_IMPORTER_HPP

#include <Assets/Material.hpp>
#include <Assets/Mesh.hpp>

namespace en
{
	struct MeshData
	{
		Handle<Mesh> mesh{};
		std::vector<Handle<Material>> materials{};
		std::vector<Handle<Texture>> textures{};
	};

	struct MeshImportProperties
	{
		bool importMaterials = true;

		bool importAlbedoTextures = true;
		bool importRoughnessTextures = true;
		bool importMetalnessTextures = true;
		bool importNormalTextures = true;

		bool importColor = true;
	};

	class Importer
	{
	public:
		Importer(const MeshImportProperties& properties, Handle<Material> defaultMaterial, Handle<Texture> defaultSRGBTexture, Handle<Texture> defaultNonSRGBTexture);

		virtual MeshData LoadMeshFromFile(const std::string& filePath, const std::string& name)
		{ 
			EN_ERROR("Importer::LoadMeshFromFile() - DO NOT USE THIS! USE A SPECIFIC IMPORTER!"); 
			return MeshData{};
		};

		uint32_t& GetMaterialCounter();

	protected:
		const Handle<Material> m_DefaultMaterial;
		const Handle<Texture> m_DefaultSRGBTexture;
		const Handle<Texture> m_DefaultNonSRGBTexture;

		const MeshImportProperties m_ImportProperties;
	};
}


#endif