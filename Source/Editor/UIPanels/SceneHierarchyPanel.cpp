#include "SceneHierarchyPanel.hpp"

namespace en
{
	void SceneHierarchyPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		
		ImGui::Begin("Scene Hierarchy", nullptr, EditorCommons::CommonFlags);

		if (ImGui::CollapsingHeader("Scene Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Text("Ambient Color:");
			ImGui::ColorEdit3("", (float*)&m_Renderer->GetScene()->m_AmbientColor, ImGuiColorEditFlags_Float);

			m_Renderer->GetScene()->m_AmbientColor = glm::clamp(m_Renderer->GetScene()->m_AmbientColor, glm::vec3(0.0f), glm::vec3(1.0f));

			SPACE();
		}

		if(ImGui::CollapsingHeader("SceneObjects", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::Spacing();

			if (ImGui::Button("Add a SceneObject", ImVec2(ImGui::GetWindowWidth() - 15.0f, 40)))
				m_Renderer->GetScene()->CreateSceneObject("New SceneObject", Mesh::GetEmptyMesh());

			else if (TriesToCopyType<SceneObject>())
			{
				const SceneObject* chosenObject = m_ChosenSceneMember->CastTo<SceneObject>();

				Handle<SceneObject> object = m_Renderer->GetScene()->CreateSceneObject(chosenObject->GetName() + "(Copy)", chosenObject->m_Mesh);

				object->m_Active   = chosenObject->m_Active;
				object->m_Position = chosenObject->m_Position;
				object->m_Rotation = chosenObject->m_Rotation;
				object->m_Scale	   = chosenObject->m_Scale;

				m_ChosenSceneMember = object.get();
			}

			SPACE();

			ImGui::PushStyleColor(ImGuiCol_Button, m_ElementColor);
			for (const auto& object : m_Renderer->GetScene()->GetAllSceneObjects())
			{
				if (ImGui::Button(object->GetName().c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
					m_ChosenSceneMember = m_Renderer->GetScene()->GetSceneObject(object->GetName()).get();
				
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
			
			else if (TriesToCopyType<PointLight>())
			{
				const PointLight* chosenPointLight = m_ChosenSceneMember->CastTo<PointLight>();

				m_ChosenSceneMember = m_Renderer->GetScene()->CreatePointLight(chosenPointLight->m_Position, chosenPointLight->m_Color, chosenPointLight->m_Intensity, chosenPointLight->m_Radius, chosenPointLight->m_Active);
			}
			
			ImGui::SameLine();

			if (ImGui::Button("+Spot Light", ImVec2(ImGui::GetWindowWidth() / 3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreateSpotLight(glm::vec3(0.0), glm::vec3(1.0, 0.0, 0.0));
			
			else if (TriesToCopyType<SpotLight>())
			{
				const SpotLight* chosenSpotLight = m_ChosenSceneMember->CastTo<SpotLight>();

				m_ChosenSceneMember = m_Renderer->GetScene()->CreateSpotLight(chosenSpotLight->m_Position, chosenSpotLight->m_Direction, chosenSpotLight->m_Color, chosenSpotLight->m_InnerCutoff, chosenSpotLight->m_OuterCutoff, chosenSpotLight->m_Range, chosenSpotLight->m_Intensity, chosenSpotLight->m_Active);
			}
			
			ImGui::SameLine();

			if (ImGui::Button("+Dir Light", ImVec2(ImGui::GetWindowWidth() / 3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreateDirectionalLight(glm::vec3(0.0, 1.0, 0.0));
			
			else if (TriesToCopyType<DirectionalLight>())
			{
				const DirectionalLight* chosenDirLight = m_ChosenSceneMember->CastTo<DirectionalLight>();

				m_ChosenSceneMember = m_Renderer->GetScene()->CreateDirectionalLight(chosenDirLight->m_Direction, chosenDirLight->m_Color, chosenDirLight->m_Intensity, chosenDirLight->m_Active);
			}
			
			SPACE();

			auto& pointLights = m_Renderer->GetScene()->m_PointLights;
			auto& spotLights  = m_Renderer->GetScene()->m_SpotLights;
			auto& dirLights   = m_Renderer->GetScene()->m_DirectionalLights;

			ImGui::PushStyleColor(ImGuiCol_Button, m_ElementColor);
			for (uint32_t i = 0U; i < pointLights.size(); i++)
			{
				if (ImGui::Button(("Point Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenSceneMember = &pointLights[i];
				}

				ImGui::Spacing();
			}
			for (uint32_t i = 0U; i < spotLights.size(); i++)
			{
				if (ImGui::Button(("Spot Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenSceneMember = &spotLights[i];
				}

				ImGui::Spacing();
			}
			for (uint32_t i = 0U; i < dirLights.size(); i++)
			{
				if (ImGui::Button(("Dir Light " + std::to_string(i)).c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
				{
					m_ChosenLightIndex = i;
					m_ChosenSceneMember = &dirLights[i];
				}

				ImGui::Spacing();
			}

			ImGui::PopStyleColor();

			ImGui::Spacing();
		}

		ImGui::End();
	}
}