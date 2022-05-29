#include "Core/EnPch.hpp"
#include "SceneHierarchyPanel.hpp"

namespace en
{
	SceneHierarchyPanel::SceneHierarchyPanel(Renderer* renderer) : m_Renderer(renderer) {}

	void SceneHierarchyPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);


		ImGui::Begin("Scene Hierarchy", nullptr, EditorCommons::CommonFlags);

		if (ImGui::CollapsingHeader("Scene Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			static float col[3];

			col[0] = m_Renderer->GetScene()->m_AmbientColor.r;
			col[1] = m_Renderer->GetScene()->m_AmbientColor.g;
			col[2] = m_Renderer->GetScene()->m_AmbientColor.b;

			ImGui::Text("Ambient Color:");
			if (ImGui::ColorEdit3("", col))
				m_Renderer->GetScene()->m_AmbientColor = glm::vec3(std::fmaxf(col[0], 0.0f), std::fmaxf(col[1], 0.0f), std::fmaxf(col[2], 0.0f));

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
					m_ChosenDirLight   = nullptr;
					m_ChosenSpotLight  = nullptr;
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

			if (ImGui::Button("+Point Light", ImVec2(ImGui::GetWindowWidth()/3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreatePointLight(glm::vec3(0.0));

			ImGui::SameLine();

			if (ImGui::Button("+Spot Light", ImVec2(ImGui::GetWindowWidth() / 3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreateSpotlight(glm::vec3(0.0), glm::vec3(1.0, 0.0, 0.0));

			ImGui::SameLine();

			if (ImGui::Button("+Dir Light", ImVec2(ImGui::GetWindowWidth() / 3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreateDirectionalLight(glm::vec3(0.0, 1.0, 0.0));

			SPACE();

			auto& pointLights = m_Renderer->GetScene()->GetAllPointLights();
			auto& spotLights = m_Renderer->GetScene()->GetAllSpotlights();
			auto& dirLights = m_Renderer->GetScene()->GetAllDirectionalLights();

			ImGui::PushStyleColor(ImGuiCol_Button, m_ElementColor);
			for (uint32_t i = 0U; i < pointLights.size(); i++)
			{
				if (ImGui::Button(("Point Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenPointLight = &pointLights[i];
					m_ChosenDirLight  = nullptr;
					m_ChosenSpotLight = nullptr;
					m_ChosenObject    = nullptr;
				}

				ImGui::Spacing();
			}
			for (uint32_t i = 0U; i < spotLights.size(); i++)
			{
				if (ImGui::Button(("Spot Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenPointLight = nullptr;
					m_ChosenDirLight = nullptr;
					m_ChosenSpotLight = &spotLights[i];
					m_ChosenObject = nullptr;
				}

				ImGui::Spacing();
			}
			for (uint32_t i = 0U; i < dirLights.size(); i++)
			{
				if (ImGui::Button(("Dir Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenPointLight = nullptr;
					m_ChosenDirLight = &dirLights[i];
					m_ChosenSpotLight = nullptr;
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