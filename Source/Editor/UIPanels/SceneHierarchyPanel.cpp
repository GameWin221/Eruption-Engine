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

			else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D, false) && m_ChosenObject && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				auto object = m_Renderer->GetScene()->CreateSceneObject(m_ChosenObject->GetName() + "(Copy)", m_ChosenObject->m_Mesh);

				object->m_Active   = m_ChosenObject->m_Active;
				object->m_Position = m_ChosenObject->m_Position;
				object->m_Rotation = m_ChosenObject->m_Rotation;
				object->m_Scale	   = m_ChosenObject->m_Scale;

				m_ChosenObject = object;
			}

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
			/*
			else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D, false) && m_ChosenPointLight && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				auto light = m_Renderer->GetScene()->CreatePointLight(m_ChosenPointLight->m_Position, m_ChosenPointLight->m_Color, m_ChosenPointLight->m_Intensity, m_ChosenPointLight->m_Radius);
				
				light->m_Active = m_ChosenPointLight->m_Active;

				m_ChosenPointLight = light;
			}
			*/
			ImGui::SameLine();

			if (ImGui::Button("+Spot Light", ImVec2(ImGui::GetWindowWidth() / 3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreateSpotlight(glm::vec3(0.0), glm::vec3(1.0, 0.0, 0.0));
			/*
			else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D, false) && m_ChosenSpotLight && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				auto light = m_Renderer->GetScene()->CreateSpotlight(m_ChosenPointLight->m_Position, m_ChosenSpotLight->m_Direction, m_ChosenSpotLight->m_Color, m_ChosenSpotLight->m_InnerCutoff, m_ChosenSpotLight->m_OuterCutoff, m_ChosenSpotLight->m_Range, m_ChosenSpotLight->m_Intensity);

				light->m_Active = m_ChosenSpotLight->m_Active;

				m_ChosenSpotLight = light;
			}
			*/
			ImGui::SameLine();

			if (ImGui::Button("+Dir Light", ImVec2(ImGui::GetWindowWidth() / 3.0f - 10.0f, 40)))
				m_Renderer->GetScene()->CreateDirectionalLight(glm::vec3(0.0, 1.0, 0.0));
			/*
			else if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_D, false) && m_ChosenDirLight && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
			{
				auto light = m_Renderer->GetScene()->CreateDirectionalLight(m_ChosenDirLight->m_Direction, m_ChosenDirLight->m_Color, m_ChosenDirLight->m_Intensity);

				light->m_Active = m_ChosenDirLight->m_Active;

				m_ChosenDirLight = light;
			}
			*/
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