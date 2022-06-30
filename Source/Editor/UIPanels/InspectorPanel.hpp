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
		InspectorPanel(SceneHierarchyPanel* sceneHierarchy, Renderer* renderer, AssetManager* assetManager)
			: m_AssetManager(assetManager), m_SceneHierarchy(sceneHierarchy), m_Renderer(renderer) {};

		void Render();

	private:
		SceneMember* m_LastChosenSceneMember = nullptr;

		SceneHierarchyPanel* m_SceneHierarchy = nullptr;
		Renderer*			 m_Renderer		  = nullptr;
		AssetManager*		 m_AssetManager   = nullptr;

		template<IsSceneMemberChild T>
		bool IsTypeSelected()
		{
			if (!m_SceneHierarchy->m_ChosenSceneMember)
				return false;

			if constexpr (std::same_as<T, SceneObject>)
				return (m_SceneHierarchy->m_ChosenSceneMember->m_Type == SceneMemberType::SceneObject);
			else if constexpr (std::same_as<T, PointLight>)
				return (m_SceneHierarchy->m_ChosenSceneMember->m_Type == SceneMemberType::PointLight);
			else if constexpr (std::same_as<T, SpotLight>)
				return (m_SceneHierarchy->m_ChosenSceneMember->m_Type == SceneMemberType::SpotLight);
			else if constexpr (std::same_as<T, DirectionalLight>)
				return (m_SceneHierarchy->m_ChosenSceneMember->m_Type == SceneMemberType::DirLight);
			else
				EN_ERROR("InspectorPanel::IsTypeSelected() - T is an uknown type!");

			return false;
		}

		void InspectSceneObject();
		void InspectPointLight();
		void InspectSpotLight();
		void InspectDirLight();
	};
}

#endif