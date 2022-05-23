#include "Core/EnPch.hpp"
#include "InspectorPanel.hpp"

en::InspectorPanel::InspectorPanel(SceneHierarchyPanel* sceneHierarchy, Renderer* renderer, AssetManager* assetManager) : m_AssetManager(assetManager), m_SceneHierarchy(sceneHierarchy), m_Renderer(renderer) {}

void en::InspectorPanel::Render()
{
	ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);

	ImGui::Begin("Inspector");

	if (m_SceneHierarchy->m_ChosenObject)
	{
		static char name[86];

		static float pos[3];
		static float rot[3];
		static float scl[3];

		static int chosenMeshIndex = 0;

		if (m_LastChosenObject != m_SceneHierarchy->m_ChosenObject)
		{
			strcpy_s(name, sizeof(char) * 86, m_SceneHierarchy->m_ChosenObject->GetName().c_str());

			chosenMeshIndex = 0;

			const auto& meshes = m_AssetManager->GetAllMeshes();

			for (int i = 0; i < meshes.size(); i++)
				if (meshes[i]->GetName() == m_SceneHierarchy->m_ChosenObject->m_Mesh->GetName())
				{
					chosenMeshIndex = i + 1;
					break;
				}

			pos[0] = m_SceneHierarchy->m_ChosenObject->m_Position.x;
			pos[1] = m_SceneHierarchy->m_ChosenObject->m_Position.y;
			pos[2] = m_SceneHierarchy->m_ChosenObject->m_Position.z;

			rot[0] = m_SceneHierarchy->m_ChosenObject->m_Rotation.x;
			rot[1] = m_SceneHierarchy->m_ChosenObject->m_Rotation.y;
			rot[2] = m_SceneHierarchy->m_ChosenObject->m_Rotation.z;

			scl[0] = m_SceneHierarchy->m_ChosenObject->m_Scale.x;
			scl[1] = m_SceneHierarchy->m_ChosenObject->m_Scale.y;
			scl[2] = m_SceneHierarchy->m_ChosenObject->m_Scale.z;
		}

		if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
		{
			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_Renderer->GetScene()->RenameSceneObject(m_SceneHierarchy->m_ChosenObject->GetName(), name);

			SPACE();

			bool deleted = false;

			if (ImGui::Button("Delete"))
			{
				m_Renderer->GetScene()->DeleteSceneObject(m_SceneHierarchy->m_ChosenObject->GetName());
				deleted = true;
			}

			SPACE();

			ImGui::DragFloat3("Position", pos, 0.1f);
			ImGui::DragFloat3("Rotation", rot, 0.1f);
			ImGui::DragFloat3("Scale", scl, 0.1f);
			ImGui::Checkbox("Active", &m_SceneHierarchy->m_ChosenObject->m_Active);

			m_SceneHierarchy->m_ChosenObject->m_Position = glm::vec3(pos[0], pos[1], pos[2]);
			m_SceneHierarchy->m_ChosenObject->m_Rotation = glm::vec3(rot[0], rot[1], rot[2]);
			m_SceneHierarchy->m_ChosenObject->m_Scale = glm::vec3(scl[0], scl[1], scl[2]);

			SPACE();

			const std::vector<Mesh*>& allMeshes = m_AssetManager->GetAllMeshes();

			std::vector<const char*> meshNames(allMeshes.size() + 1);

			meshNames[0] = "No mesh";

			for (int i = 1; i < meshNames.size(); i++)
				meshNames[i] = allMeshes[i - 1]->GetName().c_str();

			if (ImGui::Combo("Mesh", &chosenMeshIndex, meshNames.data(), meshNames.size()))
			{
				if (chosenMeshIndex == 0)
					m_SceneHierarchy->m_ChosenObject->m_Mesh = Mesh::GetEmptyMesh();
				else
					m_SceneHierarchy->m_ChosenObject->m_Mesh = m_AssetManager->GetMesh(allMeshes[chosenMeshIndex - 1]->GetName());
			}

			if (deleted)
				m_SceneHierarchy->m_ChosenObject = nullptr;
		}
	}

	else if (m_SceneHierarchy->m_ChosenPointLight)
	{
		static float pos[3];
		static float col[3];

		bool deleted = false;

		if (ImGui::Button("Delete"))
		{
			m_Renderer->GetScene()->DeletePointLight(m_SceneHierarchy->m_ChosenLightIndex);
			deleted = true;
		}

		SPACE();

		pos[0] = m_SceneHierarchy->m_ChosenPointLight->m_Position.x;
		pos[1] = m_SceneHierarchy->m_ChosenPointLight->m_Position.y;
		pos[2] = m_SceneHierarchy->m_ChosenPointLight->m_Position.z;

		col[0] = m_SceneHierarchy->m_ChosenPointLight->m_Color.r;
		col[1] = m_SceneHierarchy->m_ChosenPointLight->m_Color.g;
		col[2] = m_SceneHierarchy->m_ChosenPointLight->m_Color.b;

		ImGui::DragFloat3("Position" , pos, 0.1f);
		ImGui::ColorEdit3("Color"	 , col, ImGuiColorEditFlags_Float);
		ImGui::DragFloat ("Intensity", &m_SceneHierarchy->m_ChosenPointLight->m_Intensity, 0.1f, 0.0f, 2000.0f);
		ImGui::DragFloat ("Radius"   , &m_SceneHierarchy->m_ChosenPointLight->m_Radius   , 0.1f, 0.0f, 50.0f);
		ImGui::Checkbox  ("Active"   , &m_SceneHierarchy->m_ChosenPointLight->m_Active);

		m_SceneHierarchy->m_ChosenPointLight->m_Position = glm::vec3(pos[0], pos[1], pos[2]);
		m_SceneHierarchy->m_ChosenPointLight->m_Color	 = glm::vec3(col[0], col[1], col[2]);

		if (deleted)
			m_SceneHierarchy->m_ChosenPointLight = nullptr;
	}

	else
		ImGui::Text("No SceneObject or Light is chosen!");

	ImGui::End();

	m_LastChosenObject = m_SceneHierarchy->m_ChosenObject;
	m_LastChosenPointLight = m_SceneHierarchy->m_ChosenPointLight;
}