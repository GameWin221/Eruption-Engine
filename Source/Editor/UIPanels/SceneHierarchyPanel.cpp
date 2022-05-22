#include "Core/EnPch.hpp"
#include "SceneHierarchyPanel.hpp"

namespace en
{
	SceneHierarchyPanel::SceneHierarchyPanel(Renderer* renderer) : m_Renderer(renderer) {}

	void SceneHierarchyPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);


		ImGui::Begin("Scene Hierarchy");

		if (ImGui::CollapsingHeader("Scene Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static float col[3];

			col[0] = m_Renderer->GetScene()->m_AmbientColor.r;
			col[1] = m_Renderer->GetScene()->m_AmbientColor.g;
			col[2] = m_Renderer->GetScene()->m_AmbientColor.b;

			ImGui::Text("Ambient Color:");
			if (ImGui::ColorEdit3("", col))
				m_Renderer->GetScene()->m_AmbientColor = glm::vec3(col[0], col[1], col[2]);

			SPACE();
		}

		if(ImGui::CollapsingHeader("SceneObjects", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Spacing();

			if (ImGui::Button("Add a SceneObject", ImVec2(ImGui::GetWindowWidth() - 15.0f, 40)))
				m_Renderer->GetScene()->CreateSceneObject("New SceneObject", Mesh::GetEmptyMesh());

			SPACE();

			const std::vector<SceneObject*> sceneObjects = m_Renderer->GetScene()->GetAllSceneObjects();
			const uint32_t sceneObjectCount = sceneObjects.size();

			ImGui::PushStyleColor(ImGuiCol_Button, m_ElementColor);
			for (const auto& object : sceneObjects)
			{
				if (ImGui::Button(object->GetName().c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenPointLight = nullptr;
					m_ChosenObject = m_Renderer->GetScene()->GetSceneObject(object->GetName());
				}

				ImGui::Spacing();
			}

			ImGui::PopStyleColor();

			SPACE();
		}

		if (ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Spacing();

			if (ImGui::Button("Add New Light", ImVec2(ImGui::GetWindowWidth() - 15.0f, 40)))
				m_Renderer->GetScene()->CreatePointLight(glm::vec3(0.0));

			SPACE();

			auto& lights = m_Renderer->GetScene()->GetAllPointLights();

			ImGui::PushStyleColor(ImGuiCol_Button, m_ElementColor);
			for (uint32_t i = 0U; i < lights.size(); i++)
			{
				if (ImGui::Button(("Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenPointLight = &lights[i];
					m_ChosenObject = nullptr;
				}

				ImGui::Spacing();
			}
			
			ImGui::PopStyleColor();

			ImGui::Spacing();
		}

		ImGui::End();
	}
}