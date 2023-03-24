#pragma once

#ifndef EN_UILAYER_HPP
#define EN_UILAYER_HPP

#include <Editor/EditorImageAtlas.hpp>

#include "UIPanels/AssetManagerPanel.hpp"
#include "UIPanels/SceneHierarchyPanel.hpp"
#include "UIPanels/InspectorPanel.hpp"
#include "UIPanels/SettingsPanel.hpp"

#include <Editor/EditorCommons.hpp>
#include <Renderer/Renderer.hpp>
#include <Assets/AssetManager.hpp>

namespace en
{
	class EditorLayer
	{
	public:
		void AttachTo(Renderer* renderer, AssetManager* assetManager);

		void OnUIDraw();

		void SetVisibility(const bool& visibility);
		const bool& GetVisibility() const { return m_Visible; };

	private:
		Renderer* m_Renderer  = nullptr;

		bool m_Visible = true;
		
		Scope<EditorImageAtlas> m_Atlas;

		Scope<AssetManagerPanel  > m_AssetManagerPanel;
		Scope<SceneHierarchyPanel> m_SceneHierarchyPanel;
		Scope<InspectorPanel	 > m_InspectorPanel;
		Scope<SettingsPanel      > m_SettingsPanel;
		
		bool m_ShowLightsMenu	= true;
		bool m_ShowAssetMenu	= false;
		bool m_ShowCameraMenu	= true;
		bool m_ShowDebugMenu	= false;
		bool m_ShowSceneMenu	= true;
		bool m_ShowInspector	= true;
		bool m_ShowSettingsMenu = false;
		
		void BeginRender();
		void DrawDockspace();
		void DrawCameraMenu();
		void DrawDebugMenu();
		void EndRender();
	};
}
#endif