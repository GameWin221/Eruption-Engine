#pragma once

#ifndef EN_UILAYER_HPP
#define EN_UILAYER_HPP

#include <Editor/EditorImageAtlas.hpp>

#include "UIPanels/AssetManagerPanel.hpp"
#include "UIPanels/SceneHierarchyPanel.hpp"
#include "UIPanels/InspectorPanel.hpp"
#include "UIPanels/SettingsPanel.hpp"

namespace en
{
	class EditorLayer
	{
	public:
		void AttachTo(Renderer* renderer, AssetManager* assetManager, double* deltaTimeVar);

		void OnUIDraw();

		void SetVisibility(bool visibility);
		const bool& GetVisibility() const { return m_Visible; };

	private:
		Renderer* m_Renderer  = nullptr;
		double*	  m_DeltaTime = nullptr;

		bool m_Visible = true;

		std::unique_ptr<EditorImageAtlas> m_Atlas;

		std::unique_ptr<AssetManagerPanel  > m_AssetManagerPanel;
		std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
		std::unique_ptr<InspectorPanel	   > m_InspectorPanel;
		std::unique_ptr<SettingsPanel      > m_SettingsPanel;

		bool m_ShowLightsMenu	= true;
		bool m_ShowAssetMenu	= true;
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