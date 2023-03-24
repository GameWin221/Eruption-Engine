#pragma once

#ifndef EN_SCENEHIERARCHYPANEL_HPP
#define EN_SCENEHIERARCHYPANEL_HPP

#include <Assets/AssetManager.hpp>

#include <Renderer/Renderer.hpp>

#include <Editor/EditorCommons.hpp>
#include <Editor/EditorImageAtlas.hpp>

namespace en
{
	class SceneHierarchyPanel
	{
		friend class InspectorPanel;

	public:
		constexpr SceneHierarchyPanel(Renderer* renderer) : m_Renderer(renderer) {};

		template<IsSceneMemberChild T>
		bool TriesToCopyType()
		{
			if (!m_ChosenSceneMember)
				return false;

			bool triesToCopyType = false;

			if constexpr (std::same_as<T, SceneObject>)
				triesToCopyType = (m_ChosenSceneMember->m_Type == SceneMemberType::SceneObject);
			else if constexpr (std::same_as<T, PointLight>)
				triesToCopyType = (m_ChosenSceneMember->m_Type == SceneMemberType::PointLight);
			else if constexpr (std::same_as<T, SpotLight>)
				triesToCopyType = (m_ChosenSceneMember->m_Type == SceneMemberType::SpotLight);
			else if constexpr (std::same_as<T, DirectionalLight>)
				triesToCopyType = (m_ChosenSceneMember->m_Type == SceneMemberType::DirLight);
			else
				EN_ERROR("SceneHierarchyPanel::TriesToCopy() - T is an uknown type!");

			return ImGui::IsKeyDown(ImGuiKey_LeftShift) && ImGui::IsKeyPressed(ImGuiKey_D, false) && triesToCopyType && !ImGui::IsMouseDown(ImGuiMouseButton_Right);
		}

		void Render();

	private:
		Renderer*	       m_Renderer  = nullptr;
		EditorImageAtlas*  m_Atlas	   = nullptr;
						   
		SceneMember* m_ChosenSceneMember = nullptr;

		uint32_t m_ChosenLightIndex = 0U;

		const ImVec4 m_ElementColor = ImVec4(0.55f, 0.37f, 0.3f, 0.6f);
	};
}

#endif