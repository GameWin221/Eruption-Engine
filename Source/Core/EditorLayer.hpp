#pragma once

#ifndef EN_UILAYER_HPP
#define EN_UILAYER_HPP

#include <Renderer/Renderer.hpp>
#include <Assets/AssetManager.hpp>

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

		void OnUIDraw();

#if EN_BACKEND_API == VULKAN_BACKEND
		const std::string m_BackendAPI = "Vulkan";
#elif EN_BACKEND_API == DIRECTX_BACKEND
		const std::string m_BackendAPI = "DirectX";
#endif

	private:
		Renderer* m_Renderer;
		AssetManager* m_AssetManager;
		double* m_DeltaTime;

		ImGuiViewport* m_Viewport;
		ImGuiWindowFlags m_CommonFlags;
		ImColor m_DockedWindowBG;
		ImColor m_FreeWindowBG;

		bool m_ShowLightsMenu = true;
		bool m_ShowAssetMenu = true;
		bool m_ShowCameraMenu = true;
		bool m_ShowDebugMenu = false;

		void BeginRender();
		void DrawDockspace();
		void DrawLightsMenu();
		void DrawAssetMenu();
		void DrawCameraMenu();
		void DrawDebugMenu();
		void EndRender();

		enum struct AssetType
		{
			TypeMesh,
			TypeTexture,
			TypeMaterial
		};

		class AssetRef
		{
		public:
			AssetRef(void* assetPtr, AssetType assetType) : ptr(assetPtr), type(assetType) {}

			void* ptr;

			AssetType type;
		};

		// I know it's unsafe...
		template <typename T>
		T CastTo(void* ptr) { return reinterpret_cast<T>(ptr); };

		Material* m_ChosenMaterial = nullptr;
		Texture* m_ChosenTexture = nullptr;
		Mesh* m_ChosenMesh = nullptr;

		bool m_ShowAssetEditor = false;
		bool m_AssetEditorInit = false;
		bool m_IsCreatingMaterial = false;

		//bool m_MaterialEditorOpen = false;
		//bool m_TextureEditorOpen = false;
		//bool m_MeshEditorOpen = false;

		void EditMaterial(bool* open);
		void EditMesh(bool* open);
		void EditTexture(bool* open);
		void CreateMaterial();

		std::string TrimToTitle(std::string& path);
	};
}

#endif