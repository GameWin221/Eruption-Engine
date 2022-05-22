#pragma once

#ifndef EN_ASSETMANAGERPANEL_HPP
#define EN_ASSETMANAGERPANEL_HPP

#include <Assets/AssetManager.hpp>

#include <Editor/EditorCommons.hpp>
#include <Editor/EditorImageAtlas.hpp>

#if _WIN32
#define DEFAULT_ASSET_PATH "C:\\"
#else
#define DEFAULT_ASSET_PATH "/tmp"
#endif

namespace en
{
	class AssetManagerPanel
	{
	public:
		AssetManagerPanel(AssetManager* assetManager, EditorImageAtlas* atlas);

		void Render();

	private:
		AssetManager*	  m_AssetManager = nullptr;
		EditorImageAtlas* m_Atlas		 = nullptr;

		Material* m_ChosenMaterial = nullptr;
		Texture*  m_ChosenTexture  = nullptr;
		Mesh*	  m_ChosenMesh	   = nullptr;

		const ImColor m_AssetEditorBG = ImColor(140, 40, 140, 255);

		bool m_ShowAssetEditor = false;
		bool m_AssetEditorInit = false;

		bool m_IsCreatingMaterial = false;

		uint32_t m_MatCounter = 0U;

		void DrawAssetManager();

		void EditingMaterial(bool* open);
		void EditingMesh(bool* open);
		void EditingTexture(bool* open);
		void CreatingMaterial();

		bool AssetButtonLabeled(std::string label, glm::vec2 size, glm::uvec2 imagePos);
	};

	enum struct AssetType
	{
		Mesh,
		Texture,
		Material
	};

	class AssetRef
	{
	public:
		AssetRef(void* assetPtr, AssetType assetType) : ptr(assetPtr), type(assetType) {}

		void* ptr;

		AssetType type;

		// I know it's unsafe...
		template <typename T>
		T CastTo() { return reinterpret_cast<T>(ptr); };
	};
}

#endif