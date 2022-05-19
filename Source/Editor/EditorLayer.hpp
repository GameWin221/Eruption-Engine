#pragma once

#ifndef EN_UILAYER_HPP
#define EN_UILAYER_HPP

#include <Renderer/Renderer.hpp>
#include <Assets/AssetManager.hpp>
#include <Editor/EditorImageAtlas.hpp>

#include <portable-file-dialogs.h>

#if _WIN32
#define DEFAULT_ASSET_PATH "C:\\"
#else
#define DEFAULT_ASSET_PATH "/tmp"
#endif

namespace en
{
	class EditorLayer
	{
	public:
		void AttachTo(Renderer* renderer, AssetManager* assetManager, double* deltaTimeVar);

		~EditorLayer();

		void OnUIDraw();

#if EN_BACKEND_API == VULKAN_BACKEND
		const std::string m_BackendAPI = "Vulkan";
#elif EN_BACKEND_API == DIRECTX_BACKEND
		const std::string m_BackendAPI = "DirectX";
#endif

	private:
		Renderer*	  m_Renderer = nullptr;
		AssetManager* m_AssetManager = nullptr;
		double*		  m_DeltaTime = nullptr;

		std::unique_ptr<EditorImageAtlas> m_Atlas;

		ImGuiViewport*   m_Viewport;
		ImGuiWindowFlags m_CommonFlags;
		ImColor m_DockedWindowBG;
		ImColor m_FreeWindowBG;
		ImVec2  m_FreeWindowMinSize;
		ImVec2  m_FreeWindowMaxSize;

		bool m_ShowLightsMenu = true;
		bool m_ShowAssetMenu  = true;
		bool m_ShowCameraMenu = true;
		bool m_ShowDebugMenu  = false;

		bool m_InspectorInit = false;
		bool m_ShowSceneMenu = true;
		bool m_ShowInspector = true;

		int m_MatCounter = 0;

		void TextCentered(std::string text);

		bool ShowImageButtonLabeled(std::string label, glm::vec2 size, glm::uvec2 imagePos);

		void BeginRender();
		void DrawDockspace();

		void DrawLightsMenu();
		void DrawAssetMenu();
		void DrawCameraMenu();
		void DrawDebugMenu();

		void DrawSceneMenu();
		void DrawInspector();

		void EndRender();

		Material* m_ChosenMaterial = nullptr;
		Texture*  m_ChosenTexture = nullptr;
		Mesh*     m_ChosenMesh = nullptr;

		SceneObject* m_ChosenObject = nullptr;

		bool m_ShowAssetEditor = false;
		bool m_AssetEditorInit = false;

		bool m_IsCreatingMaterial = false;

		void EditingMaterial(bool* open);
		void EditingMesh(bool* open);
		void EditingTexture(bool* open);
		void CreatingMaterial();

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
	};
}

#endif