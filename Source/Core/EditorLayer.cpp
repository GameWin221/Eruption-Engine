#include "Core/EnPch.hpp"
#include "EditorLayer.hpp"

namespace en
{
#define SPACE() ImGui::Spacing();ImGui::Separator();ImGui::Spacing()

	void EditorLayer::AttachTo(Renderer* renderer, AssetManager* assetManager, double* deltaTimeVar)
	{
		if (renderer && deltaTimeVar && assetManager)
			EN_SUCCESS("Successfully attached the UI layer")
		else
		{
			EN_WARN("UILayer::AttachTo() - Failed to attach the UI layer!");
			if(!renderer)     EN_WARN("-The 'renderer'     value was a nullptr!");
			if(!deltaTimeVar) EN_WARN("-The 'deltaTimeVar' value was a nullptr!");
			if(!assetManager) EN_WARN("-The 'assetManager' value was a nullptr!");
			return;
		}

		m_DeltaTime = deltaTimeVar;
		m_Renderer = renderer;
		m_AssetManager = assetManager;

		m_Renderer->SetUIRenderCallback(std::bind(&EditorLayer::OnUIDraw, this));

		m_CommonFlags = ImGuiWindowFlags_None | ImGuiWindowFlags_NoCollapse;
		m_DockedWindowBG = ImColor(40, 40, 40, 100);
		m_FreeWindowBG = ImColor(40, 40, 40, 255);

		m_Viewport = ImGui::GetMainViewport();

		ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_DockingEnable;

		ImGuiStyle& style = ImGui::GetStyle();
		style.Colors[2] = m_DockedWindowBG;
		style.TabMinWidthForCloseButton = FLT_MAX;
		style.WindowMenuButtonPosition = ImGuiDir_None;
	}

	void EditorLayer::OnUIDraw()
	{
		BeginRender();

		DrawDockspace();

		if(m_ShowLightsMenu)
			DrawLightsMenu();

		if(m_ShowCameraMenu)
			DrawCameraMenu();

		ImGui::ShowDemoWindow();

		if (m_ShowAssetMenu)
			DrawAssetMenu();

		if (m_ShowDebugMenu)
			DrawDebugMenu();

		if (m_ChosenMaterial)
			EditMaterial();

		if (m_ChosenMesh)
			EditMesh();

		if (m_ChosenTexture)
			EditTexture();

		EndRender();
	}

	void EditorLayer::BeginRender()
	{
		if(m_BackendAPI == "Vulkan") 
			ImGui_ImplVulkan_NewFrame();
		//else if(m_BackendAPI == "DirectX") 
			//ImGui_ImplDirectXVulkan_NewFrame();

		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	void EditorLayer::DrawDockspace()
	{
		static ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_PassthruCentralNode;

		ImGui::SetNextWindowPos(m_Viewport->WorkPos);
		ImGui::SetNextWindowSize(m_Viewport->WorkSize);
		ImGui::SetNextWindowViewport(m_Viewport->ID);

		ImGuiWindowFlags windowFlags = 0;
		windowFlags |= ImGuiWindowFlags_NoDocking |
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

		ImGui::PushID("EruptionDockspace");
		ImGui::Begin("DockSpace Demo", nullptr, windowFlags);

		ImGui::PopStyleVar(3);

		if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_DockingEnable)
			ImGui::DockSpace(ImGui::GetID("EruptionDockspace"), ImVec2(0.0f, 0.0f), dockspaceFlags);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 10, 10 });
		ImGui::BeginMainMenuBar();

		if (ImGui::BeginMenu("Scene"))
		{
			ImGui::MenuItem("Save");

			if (ImGui::MenuItem("Save As.."))
			{
				std::string defaultPath(DEFAULT_ASSET_PATH);
				defaultPath += "UnnamedScene.enscene";

				auto file = pfd::save_file("Choose a directory for the saved scene...", defaultPath.c_str(), {"Eruption Scene File", "*.enscene"}, pfd::opt::none);

				if (file.result().length() > 0)
				{
					std::string path = file.result();

					EN_LOG("Saving a scene to \"" + path + "\"");
				}
			}
			if (ImGui::MenuItem("Load.."))
			{
				auto file = pfd::open_file("Choose a scene to load...", DEFAULT_ASSET_PATH, { "Eruption Scene File", "*.enscene" }, pfd::opt::none);

				if (file.result().size() > 0)
				{
					if (file.result()[0].length() > 0)
					{
						std::string path = file.result()[0];

						EN_LOG("Loading a scene from \"" + path + "\"");
					}
				}
			}
			if(ImGui::MenuItem("Quit"))
				exit(0);
			

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("UI Panels"))
		{
			ImGui::MenuItem("Asset Menu", "", &m_ShowAssetMenu);
			ImGui::MenuItem("Camera Menu", "", &m_ShowCameraMenu);
			ImGui::MenuItem("Lights Menu", "", &m_ShowLightsMenu);
			ImGui::MenuItem("Debug Menu", "", &m_ShowDebugMenu);
		
			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
		
		ImGui::End();
		ImGui::PopID();
		ImGui::PopStyleVar();
	}
	void EditorLayer::DrawLightsMenu()
	{
		ImGui::Begin("Lights Menu", nullptr, m_CommonFlags);

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

				ImGui::DragFloat3 ("Position" , pos[i], 0.1f);
				ImGui::ColorEdit3 ("Color"    , col[i], ImGuiColorEditFlags_Float);
				ImGui::DragFloat  ("Intensity", &lights[i].m_Intensity, 0.1f, 0.0f, 2000.0f);
				ImGui::DragFloat  ("Radius"   , &lights[i].m_Radius   , 0.1f, 0.0f, 50.0f  );
				ImGui::Checkbox   ("Active"   , &lights[i].m_Active);

				lights[i].m_Position = glm::vec3(pos[i][0], pos[i][1], pos[i][2]);
				lights[i].m_Color = glm::vec3(col[i][0], col[i][1], col[i][2]);

				ImGui::PopID();
			
				SPACE();
			}
		}

		ImGui::End();
	}
	void EditorLayer::DrawAssetMenu()
	{
		const ImVec2 buttonSize(100, 100);
		const ImVec2 assetSize(100, 100);
		const float assetsCoverage = 0.85f;
		
		ImGui::Begin("Asset Manager", nullptr, m_CommonFlags);

		ImGui::BeginChild("AssetButtons", ImVec2(ImGui::GetWindowSize().x * (1.0f - assetsCoverage), ImGui::GetWindowSize().y * 0.86f), true);

		if (ImGui::Button("Import Mesh", buttonSize))
		{
			auto file = pfd::open_file("Choose meshes to import...", DEFAULT_ASSET_PATH, { "Supported Mesh Formats", "*.gltf *.fbx *.obj" }, pfd::opt::multiselect);

			const std::vector<std::string> filePaths = file.result();

			for (const auto& path : filePaths)
			{
				std::string fileName = path.substr(path.find_last_of('/') + 1, path.length());

				m_AssetManager->LoadMesh(fileName, path);
			}
		}

		if (ImGui::Button("Import Texture", buttonSize))
		{
			auto file = pfd::open_file("Choose textures to import...", DEFAULT_ASSET_PATH, { "Supported Texture Formats", "*.png *.jpg *.jpeg" }, pfd::opt::multiselect);

			const std::vector<std::string> filePaths = file.result();

			for (const auto& path : filePaths)
			{
				std::string fileName = path.substr(path.find_last_of('/') + 1, path.length());

				m_AssetManager->LoadTexture(fileName, path);
			}
		}

		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild("AssetPreviewer", ImVec2(ImGui::GetWindowSize().x * (assetsCoverage-0.015f), ImGui::GetWindowSize().y*0.86f), true);

		std::vector<Mesh*>     meshes    = m_AssetManager->GetAllMeshes();
		std::vector<Texture*>  textures  = m_AssetManager->GetAllTextures();
		std::vector<Material*> materials = m_AssetManager->GetAllMaterials();

		std::vector<AssetRef> assets;

		for (const auto& mesh : meshes)
			assets.emplace_back(mesh, AssetType::TypeMesh);

		for (const auto& texture : textures)
			assets.emplace_back(texture, AssetType::TypeTexture);

		for (const auto& material : materials)
			assets.emplace_back(material, AssetType::TypeMaterial);
		

		int sameLineAssets = static_cast<int>(ImGui::GetWindowSize().x / (assetSize.x+10.0f));

		if (sameLineAssets <= 0) sameLineAssets = 1;

		for (int i = 0; i < assets.size(); i += sameLineAssets)
		{
			for (int j = 0; j < sameLineAssets && i+j < assets.size(); j++)
			{
				AssetRef& asset = assets[i + j];

				if (asset.type == AssetType::TypeMesh)
				{
					Mesh* mesh = CastTo<Mesh*>(asset.ptr);
					std::string name = TrimToTitle(mesh->m_FilePath);

					if (ImGui::Button(name.c_str(), assetSize))
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = mesh;
					}
				}
				else if (asset.type == AssetType::TypeTexture)
				{
					Texture* texture = CastTo<Texture*>(asset.ptr);
					std::string path = texture->GetFilePath();
					std::string name = TrimToTitle(path);

					if (ImGui::Button(name.c_str(), assetSize))
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = texture;
						m_ChosenMesh = nullptr;
					}
				}
				else if (asset.type == AssetType::TypeMaterial)
				{
					Material* material = CastTo<Material*>(asset.ptr);
					std::string name = material->m_Name;

					if (ImGui::Button(name.c_str(), assetSize))
					{
						m_ChosenMaterial = material;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = nullptr;
					}
				}
			
				if(j!=sameLineAssets-1)
					ImGui::SameLine();
			}
		}
		ImGui::EndChild();

		ImGui::End();
	}
	void EditorLayer::DrawCameraMenu()
	{
		ImGui::Begin("Camera Properties", nullptr, m_CommonFlags);
		
		static float pos[3];

		if(ImGui::InputFloat3("Position", pos))
			m_Renderer->GetMainCamera()->m_Position = glm::vec3(pos[0], pos[1], pos[2]);
		else
		{
			pos[0] = m_Renderer->GetMainCamera()->m_Position.x;
			pos[1] = m_Renderer->GetMainCamera()->m_Position.y;
			pos[2] = m_Renderer->GetMainCamera()->m_Position.z;
		}

		ImGui::SliderFloat("Pitch"   , &m_Renderer->GetMainCamera()->m_Pitch   ,-90.0f , 90.f );
		ImGui::SliderFloat("Yaw"	 , &m_Renderer->GetMainCamera()->m_Yaw	   ,-180.0f, 180.f);
		ImGui::SliderFloat("Fov"	 , &m_Renderer->GetMainCamera()->m_Fov	   , 20.0f , 110.f);
		ImGui::SliderFloat("Exposure", &m_Renderer->GetMainCamera()->m_Exposure, 0.0f  , 16.0f);
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
			case 4: modeName = "Specular Maps";	break;
			case 5: modeName = "Shininess";	    break;
			case 6: modeName = "Lights Only";   break;
			case 7: modeName = "Depth";		    break;
			case 8: modeName = "Tangent";	    break;
			case 9: modeName = "Bitangent";	    break;
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

	void EditorLayer::EditMaterial()
	{
		std::string title("Material editor - \"");
		title += m_ChosenMaterial->m_Name + "\"";

		ImGui::SetNextWindowSizeConstraints({500, 300}, {1920, 1080});
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoDocking);

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void EditorLayer::EditMesh()
	{
		std::string assetName = m_ChosenMesh->m_FilePath;
		assetName = TrimToTitle(assetName);

		std::string title("Mesh editor - \"");
		title +=  + "\"";

		ImGui::SetNextWindowSizeConstraints({ 500, 300 }, { 1920, 1080 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoDocking);

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void EditorLayer::EditTexture()
	{
		std::string assetName = m_ChosenTexture->GetFilePath();
		assetName = TrimToTitle(assetName);

		std::string title("Texture editor - \"");
		title += assetName + "\"";

		ImGui::SetNextWindowSizeConstraints({ 500, 300 }, { 1920, 1080 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoDocking);

		ImGui::End();
		ImGui::PopStyleColor();
	}

	std::string EditorLayer::TrimToTitle(std::string& path)
	{
		std::string name;

		if(path.find('/')>0)
			name = path.substr(path.find_last_of('/' ) + 1, path.size());
		else if(path.find('\\') > 0)
			name = path.substr(path.find_last_of('\\') + 1, path.size());

		return name;
	}
}