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
		SceneHierarchyPanel(Renderer* renderer);

		void Render();

	private:
		Renderer*	      m_Renderer  = nullptr;
		EditorImageAtlas* m_Atlas	  = nullptr;

		SceneObject* m_ChosenObject		= nullptr;
		PointLight*  m_ChosenPointLight = nullptr;
		uint32_t	 m_ChosenLightIndex = 0U;

		const ImVec4 m_ElementColor = ImVec4(0.55f, 0.37f, 0.3f, 0.6f);
	};
}

#endif