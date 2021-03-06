#include <Core/EnPch.hpp>
#include "EditorLayer.hpp"

namespace en
{
#define SPACE() ImGui::Spacing();ImGui::Separator();ImGui::Spacing()

	void EditorLayer::AttachTo(Renderer* renderer, AssetManager* assetManager, double* deltaTimeVar)
	{
		if (renderer && deltaTimeVar && assetManager)
			EN_SUCCESS("Attached the UI layer")
		else
		{
			EN_WARN("UILayer::AttachTo() - Failed to attach the UI layer!");
			if (!renderer)     EN_WARN("-The 'renderer'     value was a nullptr!");
			if (!deltaTimeVar) EN_WARN("-The 'deltaTimeVar' value was a nullptr!");
			if (!assetManager) EN_WARN("-The 'assetManager' value was a nullptr!");
			return;
		}

		m_DeltaTime = deltaTimeVar;
		m_Renderer = renderer;

		m_Atlas = std::make_unique<EditorImageAtlas>("Models/Atlas.png", 4, 1);

		m_Renderer->SetUIRenderCallback(std::bind(&EditorLayer::OnUIDraw, this));

		m_AssetManagerPanel   = std::make_unique<AssetManagerPanel  >(assetManager, m_Atlas.get());
		m_SceneHierarchyPanel = std::make_unique<SceneHierarchyPanel>(m_Renderer);
		m_InspectorPanel	  = std::make_unique<InspectorPanel		>(m_SceneHierarchyPanel.get(), m_Renderer, assetManager);
		m_SettingsPanel  	  = std::make_unique<SettingsPanel		>(m_Renderer);

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGuiStyle& style = ImGui::GetStyle();

		style.TabMinWidthForCloseButton = FLT_MAX;
		style.WindowMenuButtonPosition = ImGuiDir_None;
		style.WindowRounding = 6;
		style.ChildRounding = 6;
		style.FrameRounding = 6;
		style.PopupRounding = 6;
		style.ScrollbarRounding = 12;
		style.GrabRounding = 2;
		style.TabRounding = 4;
		style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
		style.ColorButtonPosition = ImGuiDir_Right;

		ImVec4* colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.50f);
		colors[ImGuiCol_ChildBg]  = ImVec4(0.18f, 0.18f, 0.18f, 0.44f);
		colors[ImGuiCol_PopupBg]  = ImVec4(0.09f, 0.09f, 0.09f, 1.00f);
		colors[ImGuiCol_Border]   = ImVec4(0.27f, 0.27f, 0.27f, 1.00f);
		colors[ImGuiCol_FrameBg]  = ImVec4(0.00f, 0.00f, 0.00f, 0.71f);
		colors[ImGuiCol_FrameBgHovered]	= ImVec4(0.46f, 0.46f, 0.46f, 0.50f);
		colors[ImGuiCol_FrameBgActive]	= ImVec4(0.30f, 0.30f, 0.30f, 1.00f);
		colors[ImGuiCol_TitleBg]		= ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
		colors[ImGuiCol_TitleBgActive]	  = ImVec4(0.00f, 0.00f, 0.00f, 1.00f);
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.71f);
		colors[ImGuiCol_MenuBarBg]     = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
		colors[ImGuiCol_ScrollbarBg]   = ImVec4(0.13f, 0.13f, 0.13f, 0.77f);
		colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.48f, 0.48f, 0.48f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
		colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.83f, 0.83f, 0.83f, 1.00f);
		colors[ImGuiCol_CheckMark]		  = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
		colors[ImGuiCol_SliderGrab]		  = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
		colors[ImGuiCol_SliderGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 1.00f);
		colors[ImGuiCol_Button]		   = ImVec4(0.86f, 0.46f, 0.26f, 0.59f);
		colors[ImGuiCol_ButtonHovered] = ImVec4(0.82f, 0.52f, 0.22f, 1.00f);
		colors[ImGuiCol_ButtonActive]  = ImVec4(0.49f, 0.26f, 0.04f, 1.00f);
		colors[ImGuiCol_Header]		   = ImVec4(0.85f, 0.46f, 0.26f, 0.59f);
		colors[ImGuiCol_HeaderHovered] = ImVec4(0.82f, 0.52f, 0.22f, 1.00f);
		colors[ImGuiCol_HeaderActive]  = ImVec4(0.49f, 0.26f, 0.04f, 1.00f);
		colors[ImGuiCol_Separator]		  = ImVec4(0.49f, 0.26f, 0.04f, 1.00f);
		colors[ImGuiCol_SeparatorHovered] = ImVec4(0.85f, 0.46f, 0.26f, 0.59f);
		colors[ImGuiCol_SeparatorActive]  = ImVec4(0.82f, 0.52f, 0.22f, 1.00f);
		colors[ImGuiCol_ResizeGrip]		   = ImVec4(0.49f, 0.26f, 0.04f, 1.00f);
		colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.85f, 0.46f, 0.26f, 0.59f);
		colors[ImGuiCol_ResizeGripActive]  = ImVec4(0.82f, 0.52f, 0.22f, 1.00f);
		colors[ImGuiCol_Tab]			   = ImVec4(0.72f, 0.25f, 0.01f, 0.59f);
		colors[ImGuiCol_TabHovered]		    = ImVec4(0.89f, 0.43f, 0.19f, 0.59f);
		colors[ImGuiCol_TabActive]		    = ImVec4(0.99f, 0.54f, 0.11f, 1.00f);
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.73f, 0.25f, 0.01f, 0.59f);
		colors[ImGuiCol_DockingPreview]		= ImVec4(0.73f, 0.25f, 0.01f, 0.59f);
	}

	void EditorLayer::OnUIDraw()
	{
		BeginRender();

		DrawDockspace();

		// UI Panels
		if (m_ShowCameraMenu)
			DrawCameraMenu();

		if (m_ShowDebugMenu)
			DrawDebugMenu();

		if (m_ShowSceneMenu)
			m_SceneHierarchyPanel->Render();

		if (m_ShowInspector)
			m_InspectorPanel->Render();

		if (m_ShowAssetMenu)
			m_AssetManagerPanel->Render();

		if (m_ShowSettingsMenu)
			m_SettingsPanel->Render();

		EndRender();
	}

	void EditorLayer::SetVisibility(const bool& visibility)
	{
		m_Visible = visibility;

		if(m_Visible)
			m_Renderer->SetUIRenderCallback(std::bind(&EditorLayer::OnUIDraw, this));
		else
			m_Renderer->SetUIRenderCallback(nullptr);
	}

	void EditorLayer::BeginRender()
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();

		ImGui::NewFrame();
	}
	void EditorLayer::DrawDockspace()
	{
		static const auto* viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGuiWindowFlags windowFlags =
			ImGuiWindowFlags_NoDocking |
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoBringToFrontOnFocus |
			ImGuiWindowFlags_NoNavFocus |
			ImGuiWindowFlags_NoBackground;


		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace Demo", nullptr, windowFlags);

		ImGui::PopStyleVar(3);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
			ImGui::DockSpace(ImGui::GetID("EruptionDockspace"), ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 10 });
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Scene"))
		{
			ImGui::MenuItem("Save");

			if (ImGui::MenuItem("Save As.."))
			{
				/*
				std::string defaultPath(DEFAULT_ASSET_PATH);
				defaultPath += "UnnamedScene.enscene";

				auto file = pfd::save_file("Choose a directory for the saved scene...", defaultPath.c_str(), {"Eruption Scene File", "*.enscene"}, pfd::opt::none);

				if (file.result().length() > 0)
				{
					std::string path = file.result();

					EN_LOG("Saving a scene to \"" + path + "\"");
				}
				*/
			}
			if (ImGui::MenuItem("Load.."))
			{
				/*
				auto file = pfd::open_file("Choose a scene to load...", DEFAULT_ASSET_PATH, { "Eruption Scene File", "*.enscene" }, pfd::opt::none);

				if (file.result().size() > 0)
				{
					if (file.result()[0].length() > 0)
					{
						std::string path = file.result()[0];

						EN_LOG("Loading a scene from \"" + path + "\"");
					}
				}
				*/
			}
			if (ImGui::MenuItem("Quit"))
				exit(0);


			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("UI Panels"))
		{
			ImGui::MenuItem("Asset Menu", "", &m_ShowAssetMenu);
			ImGui::MenuItem("Camera Menu", "", &m_ShowCameraMenu);
			ImGui::MenuItem("Lights Menu", "", &m_ShowLightsMenu);
			ImGui::MenuItem("Debug Menu", "", &m_ShowDebugMenu);

			ImGui::MenuItem("Scene Menu", "", &m_ShowSceneMenu);
			ImGui::MenuItem("Inspector", "", &m_ShowInspector);
			ImGui::MenuItem("Settings", "", &m_ShowSettingsMenu);

			if (ImGui::MenuItem("Toggle UI (Shift+I)", "", &m_Visible))
				SetVisibility(m_Visible);

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();

		ImGui::End();
		ImGui::PopStyleVar();
	}
	void EditorLayer::DrawCameraMenu()
	{
		ImGui::Begin("Camera Properties", nullptr, EditorCommons::CommonFlags);

		ImGui::InputFloat3("Position", (float*)&m_Renderer->GetMainCamera()->m_Position);

		ImGui::SliderFloat("Fov", &m_Renderer->GetMainCamera()->m_Fov, 20.0f, 110.f);
		ImGui::SliderFloat("Exposure", &m_Renderer->GetMainCamera()->m_Exposure, 0.0f, 16.0f);
		ImGui::Checkbox("Dynamically Scaled", &m_Renderer->GetMainCamera()->m_DynamicallyScaled);

		Camera* cam = m_Renderer->GetMainCamera();

		static int res[2] = { cam->m_Size.x, cam->m_Size.y };

		if (!cam->m_DynamicallyScaled)
		{
			ImGui::InputInt2("Target resolution", res);
			if (res[0] <= 0) res[0] = 1;
			if (res[1] <= 0) res[1] = 1;

			cam->m_Size = glm::vec2(res[0], res[1]);
		}
		else
		{
			res[0] = static_cast<int>(cam->m_Size.x);
			res[1] = static_cast<int>(cam->m_Size.y);
		}

		ImGui::End();
	}
	void EditorLayer::DrawDebugMenu()
	{
		ImGui::Begin("Debug Menu");

		if (ImGui::CollapsingHeader("Stats"))
		{
			ImGui::Text(("FPS: " + std::to_string(1.0 / *m_DeltaTime)).c_str());
			ImGui::Text((std::to_string(*m_DeltaTime) + "ms/frame").c_str());
		}

		SPACE();

		if (ImGui::CollapsingHeader("Debug Views"))
		{
			static int mode = 0;

			if (ImGui::SliderInt("Debug View", &mode, 0, 7))
				m_Renderer->SetDebugMode(mode);

			std::string modeName = "";

			switch (mode)
			{
				case 0: modeName = "No Debug View"; break;
				case 1: modeName = "Albedo";        break;
				case 2: modeName = "Normals";       break;
				case 3: modeName = "Position";      break;
				case 4: modeName = "Roughness Maps";break;
				case 5: modeName = "Metalness";	    break;
				case 6: modeName = "Depth";			break;
				default: modeName = "Unknown Mode"; break;
			}

			ImGui::Text(("View: " + modeName).c_str());
		}

		ImGui::End();
	}
	void EditorLayer::EndRender()
	{
		ImGui::Render();
	}
}