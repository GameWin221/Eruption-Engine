#include "Core/EnPch.hpp"
#include "UILayer.hpp"

namespace en
{
	void UILayer::AttachTo(Renderer* renderer, double* deltaTimeVar)
	{
		m_DeltaTime = deltaTimeVar;
		m_Renderer = renderer;

		m_Renderer->SetUIRenderCallback(std::bind(&UILayer::OnUIDraw, this));
	}

	void UILayer::OnUIDraw()
	{
		DrawLightsMenu();
		DrawDebugMenu();
	}
	
	void UILayer::DrawLightsMenu()
	{
		ImGui::Begin("Lights Menu");

		std::array<PointLight, MAX_LIGHTS>& lights = m_Renderer->GetPointLights();

		static bool posInit = false, colInit = false;

		static float pos[MAX_LIGHTS][3];
		static float col[MAX_LIGHTS][3];

		// Initialise positions and colors if aren't initialised yet
		if (!posInit)
		{
			for (int i = 0; auto & position : pos)
			{
				position[0] = lights[i].m_Position.x;
				position[1] = lights[i].m_Position.y;
				position[2] = lights[i].m_Position.z;
				i++;
			}
			posInit = true;
		}
		if (!colInit)
		{
			for (int i = 0; auto & color : col)
			{
				color[0] = lights[i].m_Color.x;
				color[1] = lights[i].m_Color.y;
				color[2] = lights[i].m_Color.z;
				i++;
			}
			colInit = true;
		}

		for (int i = 0; i < MAX_LIGHTS; i++)
		{
			std::string label = "Light " + std::to_string(i);

			if (ImGui::CollapsingHeader(label.c_str()))
			{
				ImGui::PushID(&lights[i]);

				ImGui::InputFloat3("Position", pos[i]);
				ImGui::ColorEdit3 ("Color", col[i]);
				ImGui::SliderFloat3("Normalized Color", col[i], 0.0f, 1.0f);
				ImGui::SliderFloat("Intensity", &lights[i].m_Intensity, 0, 8.0f);
				ImGui::SliderFloat("Radius", &lights[i].m_Radius, 0, 20.0f);
				ImGui::Checkbox   ("Active", &lights[i].m_Active);

				lights[i].m_Position = glm::vec3(pos[i][0], pos[i][1], pos[i][2]);
				lights[i].m_Color = glm::vec3(col[i][0], col[i][1], col[i][2]);

				ImGui::PopID();

				ImGui::Separator();
				ImGui::NewLine();
			}
		}


		//ImGui::ShowDemoWindow();

		ImGui::End();
	}

	void UILayer::DrawDebugMenu()
	{
		static int mode = 0;

		ImGui::Begin("Debug Menu");

		if (ImGui::SliderInt("Debug View", &mode, 0, 7))
			m_Renderer->SetDebugMode(mode);

		std::string modeName;

		switch (mode)
		{
		case 0: modeName = "No Debug View"; break;
		case 1: modeName = "Albedo";        break;
		case 2: modeName = "Normals";       break;
		case 3: modeName = "Position";      break;
		case 4: modeName = "Specular Maps";	break;
		case 5: modeName = "Shininess";	    break;
		case 6: modeName = "Lights Only";   break;
		case 7: modeName = "Depth";		    break;
		default: modeName = "Unknown Mode"; break;
		}

		ImGui::Text(modeName.c_str());

		ImGui::Text(std::to_string(1.0 / *m_DeltaTime).c_str());

		ImGui::End();
	}
}