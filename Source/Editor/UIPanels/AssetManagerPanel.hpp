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

		Asset* m_ChosenAsset = nullptr;

		const ImColor m_AssetEditorBG = ImColor(40, 40, 40, 255);

		bool m_ShowAssetEditor = false;
		bool m_AssetEditorInit = false;

		bool m_IsCreatingMaterial = false;
		bool m_IsImportingMesh = false;
		bool m_IsImportingTexture = false;

		uint32_t m_MatCounter = 0U;

		void DrawAssetManager();

		void ImportingMesh(bool* open);
		void ImportingTexture(bool* open);

		void EditingMaterial(bool* open);
		void EditingMesh(bool* open);
		void EditingTexture(bool* open);
		void CreatingMaterial();

		bool AssetButtonLabeled(const std::string& label, const  glm::vec2& size, const glm::uvec2& imagePos);

		template<IsAssetChild T>
		bool IsTypeSelected()
		{
			if (!m_ChosenAsset)
				return false;

			if constexpr (std::same_as<T, Mesh>)
				return (m_ChosenAsset->m_Type == AssetType::Mesh);
			else if constexpr (std::same_as<T, SubMesh>)
				return (m_ChosenAsset->m_Type == AssetType::SubMesh);
			else if constexpr (std::same_as<T, Texture>)
				return (m_ChosenAsset->m_Type == AssetType::Texture);
			else if constexpr (std::same_as<T, Material>)
				return (m_ChosenAsset->m_Type == AssetType::Material);
			else
				EN_ERROR("AssetManagerPanel::IsTypeSelected() - T is an uknown type!");
		
			return false;
		}
	};
}

#endif