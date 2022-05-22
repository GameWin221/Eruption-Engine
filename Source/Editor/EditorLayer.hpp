#pragma once

#ifndef EN_UILAYER_HPP
#define EN_UILAYER_HPP

#include <Editor/EditorImageAtlas.hpp>

#include "UIPanels/AssetManagerPanel.hpp"
#include "UIPanels/SceneHierarchyPanel.hpp"
#include "UIPanels/InspectorPanel.hpp"

namespace en
{
	class EditorLayer
	{
	public:
		void AttachTo(Renderer* renderer, AssetManager* assetManager, double* deltaTimeVar);

		void OnUIDraw();

	private:
		Renderer* m_Renderer  = nullptr;
		double*	  m_DeltaTime = nullptr;

		std::unique_ptr<EditorImageAtlas> m_Atlas;

		std::unique_ptr<AssetManagerPanel>   m_AssetManagerPanel;
		std::unique_ptr<SceneHierarchyPanel> m_SceneHierarchyPanel;
		std::unique_ptr<InspectorPanel>		 m_InspectorPanel;

		bool m_ShowLightsMenu = true;
		bool m_ShowAssetMenu  = true;
		bool m_ShowCameraMenu = true;
		bool m_ShowDebugMenu  = false;
		bool m_ShowSceneMenu  = true;
		bool m_ShowInspector  = true;

		void BeginRender();
		void DrawDockspace();
		void DrawCameraMenu();
		void DrawDebugMenu();
		void EndRender();
	};
}
#endif