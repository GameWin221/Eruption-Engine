#include "Core/EnPch.hpp"
#include "InspectorPanel.hpp"

namespace en
{
	void InspectorPanel::Render()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);

		ImGui::Begin("Inspector", nullptr, EditorCommons::CommonFlags);

		if (IsTypeSelected<SceneObject>())
			InspectSceneObject();

		else if (IsTypeSelected<PointLight>())
			InspectPointLight();

		else if (IsTypeSelected<SpotLight>())
			InspectSpotLight();

		else if (IsTypeSelected<DirectionalLight>())
			InspectDirLight();

		else
			ImGui::Text("No SceneMember is chosen!");

		ImGui::End();

	}

	void InspectorPanel::InspectSceneObject()
	{
		static char name[86];

		static int chosenMeshIndex = 0;

		SceneObject* chosenSceneObject = m_SceneHierarchy->m_ChosenSceneMember->CastTo<SceneObject>();

		if (m_LastChosenSceneMember != m_SceneHierarchy->m_ChosenSceneMember)
		{
			strcpy_s(name, sizeof(char) * 86, chosenSceneObject->GetName().c_str());

			chosenMeshIndex = 0;

			const auto& meshes = m_AssetManager->GetAllMeshes();

			for (int i = 0; i < meshes.size(); i++)
				if (meshes[i]->GetName() == chosenSceneObject->m_Mesh->GetName())
				{
					chosenMeshIndex = i + 1;
					break;
				}
		}

		if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_Renderer->GetScene()->RenameSceneObject(chosenSceneObject->GetName(), name);

			SPACE();

			bool deleted = false;

			if (ImGui::Button("Delete"))
				EditorCommons::OpenYesNoDecision();

			if (EditorCommons::YesNoDecision() || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
			{
				m_Renderer->GetScene()->DeleteSceneObject(chosenSceneObject->GetName());
				deleted = true;
			}

			SPACE();

			ImGui::DragFloat3("Position", (float*)&chosenSceneObject->m_Position, 0.1f);
			ImGui::DragFloat3("Rotation", (float*)&chosenSceneObject->m_Rotation, 0.1f);
			ImGui::DragFloat3("Scale", (float*)&chosenSceneObject->m_Scale, 0.1f);
			ImGui::Checkbox("Active", &chosenSceneObject->m_Active);

			SPACE();

			const std::vector<Mesh*>& allMeshes = m_AssetManager->GetAllMeshes();

			std::vector<const char*> meshNames(allMeshes.size() + 1);

			meshNames[0] = "No mesh";

			for (int i = 1; i < meshNames.size(); i++)
				meshNames[i] = allMeshes[i - 1]->GetName().c_str();

			if (ImGui::Combo("Mesh", &chosenMeshIndex, meshNames.data(), meshNames.size()))
			{
				if (chosenMeshIndex == 0)
					chosenSceneObject->m_Mesh = Mesh::GetEmptyMesh();
				else
					chosenSceneObject->m_Mesh = m_AssetManager->GetMesh(allMeshes[chosenMeshIndex - 1]->GetName());
			}

			if (deleted)
				m_SceneHierarchy->m_ChosenSceneMember = nullptr;
		}

		m_LastChosenSceneMember = m_SceneHierarchy->m_ChosenSceneMember;
	}
	void InspectorPanel::InspectPointLight()
	{
		PointLight* chosenPointLight = m_SceneHierarchy->m_ChosenSceneMember->CastTo<PointLight>();

		if (ImGui::Button("Delete"))
			EditorCommons::OpenYesNoDecision();

		if (EditorCommons::YesNoDecision() || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			m_Renderer->GetScene()->DeletePointLight(m_SceneHierarchy->m_ChosenLightIndex);
			m_SceneHierarchy->m_ChosenSceneMember = nullptr;
		}

		SPACE();

		ImGui::DragFloat3("Position", (float*)&chosenPointLight->m_Position, 0.1f);
		ImGui::ColorEdit3("Color", (float*)&chosenPointLight->m_Color, ImGuiColorEditFlags_Float);
		ImGui::DragFloat("Intensity", &chosenPointLight->m_Intensity, 0.1f, 0.0f, 2000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Radius", &chosenPointLight->m_Radius, 0.1f, 0.0f, 50.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Checkbox("Active", &chosenPointLight->m_Active);

		ImGui::Checkbox("Cast Shadows", &chosenPointLight->m_CastShadows);

		if (chosenPointLight->m_CastShadows)
		{
			ImGui::DragFloat("Shadow Softness", &chosenPointLight->m_ShadowSoftness, 1.0f, 1.0f, 10.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("PCF Sample Count", &chosenPointLight->m_PCFSampleRate, 0.5f, 1, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
		}



		chosenPointLight->m_Color = glm::clamp(chosenPointLight->m_Color, glm::vec3(0.0f), glm::vec3(1.0f));

		m_LastChosenSceneMember = m_SceneHierarchy->m_ChosenSceneMember;
	}
	void InspectorPanel::InspectSpotLight()
	{
		static float outerAngle;
		static float innerAngle;

		SpotLight* chosenSpotLight = m_SceneHierarchy->m_ChosenSceneMember->CastTo<SpotLight>();

		if (ImGui::Button("Delete"))
			EditorCommons::OpenYesNoDecision();

		if (EditorCommons::YesNoDecision() || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			m_Renderer->GetScene()->DeleteSpotLight(m_SceneHierarchy->m_ChosenLightIndex);
			m_SceneHierarchy->m_ChosenSceneMember = nullptr;
		}

		SPACE();

		ImGui::DragFloat3("Position", (float*)&chosenSpotLight->m_Position, 0.1f);
		ImGui::DragFloat3("Direction", (float*)&chosenSpotLight->m_Direction, 0.1f, -1.0f, 1.0f);
		ImGui::ColorEdit3("Color", (float*)&chosenSpotLight->m_Color, ImGuiColorEditFlags_Float);

		chosenSpotLight->m_Direction = glm::normalize(chosenSpotLight->m_Direction);

		chosenSpotLight->m_Color = glm::clamp(chosenSpotLight->m_Color, glm::vec3(0.0f), glm::vec3(1.0f));

		ImGui::DragFloat("Intensity", &chosenSpotLight->m_Intensity, 0.1f, 0.0f, 2000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Range", &chosenSpotLight->m_Range, 0.5f, 0.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

		outerAngle = chosenSpotLight->m_OuterCutoff * 180.0f;
		innerAngle = chosenSpotLight->m_InnerCutoff * 180.0f;

		ImGui::DragFloat("Outer Angle", &outerAngle, 1.0f, 0.0f, 150.0, "%.3f", ImGuiSliderFlags_AlwaysClamp);

		innerAngle = std::fmin(innerAngle, outerAngle);

		ImGui::DragFloat("Inner Angle", &innerAngle, 1.0f, 0.0f, outerAngle, "%.3f", ImGuiSliderFlags_AlwaysClamp);

		chosenSpotLight->m_OuterCutoff = outerAngle / 180.0f;
		chosenSpotLight->m_InnerCutoff = innerAngle / 180.0f;

		ImGui::Checkbox("Active", &chosenSpotLight->m_Active);
		ImGui::Checkbox("Cast Shadows", &chosenSpotLight->m_CastShadows);

		if (chosenSpotLight->m_CastShadows)
		{
			ImGui::DragFloat("Shadow Softness", &chosenSpotLight->m_ShadowSoftness, 1.0f, 1.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("PCF Sample Count", &chosenSpotLight->m_PCFSampleRate, 0.5f, 1, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
		}


		m_LastChosenSceneMember = m_SceneHierarchy->m_ChosenSceneMember;
	}
	void InspectorPanel::InspectDirLight()
	{
		DirectionalLight* chosenDirLight = m_SceneHierarchy->m_ChosenSceneMember->CastTo<DirectionalLight>();

		if (ImGui::Button("Delete"))
			EditorCommons::OpenYesNoDecision();

		if (EditorCommons::YesNoDecision() || ImGui::IsKeyPressed(ImGuiKey_Delete, false))
		{
			m_Renderer->GetScene()->DeleteDirectionalLight(m_SceneHierarchy->m_ChosenLightIndex);
			m_SceneHierarchy->m_ChosenSceneMember = nullptr;
		}

		SPACE();

		ImGui::DragFloat3("Direction", (float*)&chosenDirLight->m_Direction, 0.1f, -1.0, 1.0);
		ImGui::ColorEdit3("Color", (float*)&chosenDirLight->m_Color, ImGuiColorEditFlags_Float);
		ImGui::DragFloat("Intensity", &chosenDirLight->m_Intensity, 0.1f, 0.0f, 2000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
		ImGui::Checkbox("Active", &chosenDirLight->m_Active);

		ImGui::Checkbox("Cast Shadows", &chosenDirLight->m_CastShadows);

		if (chosenDirLight->m_CastShadows)
		{
			ImGui::DragFloat("Shadow Softness", &chosenDirLight->m_ShadowSoftness, 1.0f, 1.0f, 20.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
			ImGui::DragInt("PCF Sample Count", &chosenDirLight->m_PCFSampleRate, 0.5f, 1, 4, "%d", ImGuiSliderFlags_AlwaysClamp);
		}

		chosenDirLight->m_Color = glm::clamp(chosenDirLight->m_Color, glm::vec3(0.0f), glm::vec3(1.0f));

		chosenDirLight->m_Direction = glm::normalize(chosenDirLight->m_Direction);

		m_LastChosenSceneMember = m_SceneHierarchy->m_ChosenSceneMember;
	}
}