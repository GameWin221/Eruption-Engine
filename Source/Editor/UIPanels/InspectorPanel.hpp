#pragma once

#ifndef EN_INSPECTORPANEL_HPP
#define EN_INSPECTORPANEL_HPP

#include <Assets/AssetManager.hpp>
#include <Renderer/Renderer.hpp>

#include <Editor/EditorCommons.hpp>
#include <Editor/EditorImageAtlas.hpp>

#include "SceneHierarchyPanel.hpp"

namespace en
{
	class InspectorPanel
	{
	public:
		InspectorPanel(SceneHierarchyPanel* sceneHierarchy, Renderer* renderer, AssetManager* assetManager);

		void Render();

	private:
		void* m_LastChosen = nullptr;

		SceneHierarchyPanel* m_SceneHierarchy = nullptr;
		Renderer*			 m_Renderer		  = nullptr;
		AssetManager*		 m_AssetManager   = nullptr;

		void InspectSceneObject();
		void InspectPointLight();
		void InspectSpotlight();
		void InspectDirLight();
	};
}

#endif