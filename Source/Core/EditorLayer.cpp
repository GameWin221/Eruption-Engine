#include <Core/EnPch.hpp>
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

		// UI Panels

		if(m_ShowLightsMenu)
			DrawLightsMenu();

		if(m_ShowCameraMenu)
			DrawCameraMenu();

		ImGui::ShowDemoWindow();

		if (m_ShowAssetMenu)
			DrawAssetMenu();

		if (m_ShowDebugMenu)
			DrawDebugMenu();

		
		// Asset Editors
		if (m_ChosenMaterial && m_ShowAssetEditor)
			EditMaterial(&m_ShowAssetEditor);
		
		else if (m_ChosenMesh && m_ShowAssetEditor)
			EditMesh(&m_ShowAssetEditor);

		else if (m_ChosenTexture && m_ShowAssetEditor)
			EditTexture(&m_ShowAssetEditor);

		if (m_ChosenMaterial && m_IsCreatingMaterial)
			CreateMaterial();

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

		if (ImGui::Button("Import Mesh", buttonSize) && !m_IsCreatingMaterial)
		{
			auto file = pfd::open_file("Choose meshes to import...", DEFAULT_ASSET_PATH, { "Supported Mesh Formats", "*.gltf *.fbx *.obj" }, pfd::opt::multiselect);

			const std::vector<std::string> filePaths = file.result();

			for (const auto& path : filePaths)
			{
				std::string fileName = path.substr(path.find_last_of('/') + 1, path.length());

				m_AssetManager->LoadMesh(fileName, path);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Import Texture", buttonSize) && !m_IsCreatingMaterial)
		{
			auto file = pfd::open_file("Choose textures to import...", DEFAULT_ASSET_PATH, { "Supported Texture Formats", "*.png *.jpg *.jpeg" }, pfd::opt::multiselect);

			const std::vector<std::string> filePaths = file.result();

			for (const auto& path : filePaths)
			{
				std::string fileName = path.substr(path.find_last_of('/') + 1, path.length());

				m_AssetManager->LoadTexture(fileName, path);
			}
		}

		static int matCounter = 0;

		if (ImGui::Button("Create Material", buttonSize) && !m_IsCreatingMaterial)
		{
			m_IsCreatingMaterial = true;

			std::string newName{ "New Material" };

			if (matCounter > 0)
				newName.append("[" + std::to_string(matCounter) + "]");

			m_AssetManager->CreateMaterial(newName);

			m_ChosenMaterial = m_AssetManager->GetMaterial(newName);

			matCounter++;
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

					if (ImGui::Button(name.c_str(), assetSize) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = mesh;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}
				}
				else if (asset.type == AssetType::TypeTexture)
				{
					Texture* texture = CastTo<Texture*>(asset.ptr);
					std::string path = texture->GetFilePath();
					std::string name = TrimToTitle(path);

					if (ImGui::Button(name.c_str(), assetSize) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = texture;
						m_ChosenMesh = nullptr;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}
				}
				else if (asset.type == AssetType::TypeMaterial)
				{
					Material* material = CastTo<Material*>(asset.ptr);
					std::string name = material->GetName();

					if (ImGui::Button(name.c_str(), assetSize) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = material;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = nullptr;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
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

	void EditorLayer::EditMaterial(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints({500, 300}, {1920, 1080});
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin(("Material editor - \"" + m_ChosenMaterial->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { m_ChosenMaterial->m_Color.r, m_ChosenMaterial->m_Color.y, m_ChosenMaterial->m_Color.z};

			static char name[86];

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenMaterial->GetName().c_str());
				m_AssetEditorInit = false;
			}

			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_AssetManager->RenameMaterial(m_ChosenMaterial->GetName(), name);

			if (ImGui::ColorEdit3("Color: ", col))
			{
				m_ChosenMaterial->m_Color.r = col[0];
				m_ChosenMaterial->m_Color.y = col[1];
				m_ChosenMaterial->m_Color.z = col[2];
			}
			else
			{
				col[0] = m_ChosenMaterial->m_Color.r;
				col[1] = m_ChosenMaterial->m_Color.y;
				col[2] = m_ChosenMaterial->m_Color.z;
			}


			ImGui::DragFloat("Shininess: ", &m_ChosenMaterial->m_Shininess, 0.5f, 1.0f, 512.0f);
			ImGui::DragFloat("Normal Strength: ", &m_ChosenMaterial->m_NormalStrength, 0.02f, 0.0f, 1.0f);

			const std::vector<Texture*>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			SPACE();

			// Albedo texture
			{ 
				ImGui::PushID("Albedo");
				ImGui::Text(("Albedo Texture: " + m_ChosenMaterial->GetAlbedoTexture()->GetFilePath()).c_str());
				ImGui::Text("Choose new albedo texture: ");

				static int chosenAlbedoIndex = 0;
				ImGui::Combo("Textures", &chosenAlbedoIndex, textureNames.data(), textureNames.size());

				ImGui::SameLine();

				if (ImGui::Button("Update Texture"))
				{
					if (chosenAlbedoIndex == 0)
						m_ChosenMaterial->SetAlbedoTexture(Texture::GetWhiteSRGBTexture());
					else
						m_ChosenMaterial->SetAlbedoTexture(m_AssetManager->GetTexture(allTextures[chosenAlbedoIndex - 1]->GetName()));
				}
				ImGui::PopID();
			}

			SPACE();

			// Specular texture
			{ 
				ImGui::PushID("Specular");
				ImGui::Text(("Specular Texture: " + m_ChosenMaterial->GetSpecularTexture()->GetFilePath()).c_str());
				ImGui::Text("Choose new specular texture: ");

				static int chosenSpecularIndex = 0;
				ImGui::Combo("Textures", &chosenSpecularIndex, textureNames.data(), textureNames.size());

				ImGui::SameLine();

				if (ImGui::Button("Update Texture"))
				{
					if (chosenSpecularIndex == 0)
						m_ChosenMaterial->SetSpecularTexture(Texture::GetGreyNonSRGBTexture());
					else
						m_ChosenMaterial->SetSpecularTexture(m_AssetManager->GetTexture(allTextures[chosenSpecularIndex - 1]->GetName()));
				}
				ImGui::PopID();
			}
			
			SPACE();

			// Normal texture
			{
				ImGui::PushID("Normal");
				ImGui::Text(("Normal Texture: " + m_ChosenMaterial->GetNormalTexture()->GetFilePath()).c_str());
				ImGui::Text("Choose new normal texture: ");

				static int chosenNormalIndex = 0;
				ImGui::Combo("Textures", &chosenNormalIndex, textureNames.data(), textureNames.size());

				ImGui::SameLine();

				if (ImGui::Button("Update Texture"))
				{
					if (chosenNormalIndex == 0)
						m_ChosenMaterial->SetNormalTexture(Texture::GetNormalTexture());
					else
						m_ChosenMaterial->SetNormalTexture(m_AssetManager->GetTexture(allTextures[chosenNormalIndex - 1]->GetName()));
				}
				ImGui::PopID();
			}
		}

		ImGui::End();


		ImGui::PopStyleColor();
	}
	void EditorLayer::EditMesh(bool* open)
	{
		std::string assetName = m_ChosenMesh->m_FilePath;
		assetName = TrimToTitle(assetName);

		std::string title("Mesh editor - \"");
		title += assetName + "\"";

		ImGui::SetNextWindowSizeConstraints({ 500, 300 }, { 1920, 1080 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin(title.c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static char name[86];

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenMesh->GetName().c_str());
				m_AssetEditorInit = false;
			}

			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_AssetManager->RenameMesh(m_ChosenMesh->GetName(), name);

			ImGui::Text(("Path: " + m_ChosenMesh->m_FilePath).c_str());

			for (int id = 0; auto& subMesh : m_ChosenMesh->m_SubMeshes)
			{
				ImGui::PushID(id);

				if (ImGui::CollapsingHeader(("Submesh [" + std::to_string(id) + "]").c_str()))
				{
					ImGui::Text(("Indices: " + std::to_string(subMesh.m_VertexBuffer->GetSize())).c_str());
					ImGui::Text(("Material: " + subMesh.m_Material->GetName()).c_str());

					ImGui::Spacing();

					ImGui::Text("Choose new material: ");
					const std::vector<Material*>& allMaterials = m_AssetManager->GetAllMaterials();

					std::vector<const char*> materialNames(allMaterials.size()+1);

					materialNames[0] = "No material";

					for (int i = 1; i < materialNames.size(); i++)
						materialNames[i] = allMaterials[i-1]->GetName().c_str();
					
					static int chosenMaterialIndex = 0;
					ImGui::Combo("Materials", &chosenMaterialIndex, materialNames.data(), materialNames.size());

					ImGui::SameLine();

					if (ImGui::Button("Update Material"))
					{
						if (chosenMaterialIndex == 0)
							subMesh.m_Material = Material::GetDefaultMaterial();
						else
							subMesh.m_Material = m_AssetManager->GetMaterial(allMaterials[chosenMaterialIndex-1]->GetName());
					}

					SPACE();
				}
			
				ImGui::PopID();

				id++;
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void EditorLayer::EditTexture(bool* open)
	{
		std::string assetName = m_ChosenTexture->GetFilePath();
		assetName = TrimToTitle(assetName);

		ImGui::SetNextWindowSizeConstraints({ 500, 300 }, { 1920, 1080 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin(("Texture editor - \"" + assetName + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::Text(("Name: " + assetName).c_str());
			ImGui::Text(("Path: " + m_ChosenTexture->GetFilePath()).c_str());
			// More editing
		}

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void EditorLayer::CreateMaterial()
	{
		ImGui::SetNextWindowSizeConstraints({ 500, 300 }, { 1920, 1080 });
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin("Creating a new material", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { m_ChosenMaterial->m_Color.r, m_ChosenMaterial->m_Color.y, m_ChosenMaterial->m_Color.z };

			static char name[86];

			ImGui::InputText("Name: ", name, 86);

			if (ImGui::ColorEdit3("Color: ", col))
			{
				m_ChosenMaterial->m_Color.r = col[0];
				m_ChosenMaterial->m_Color.y = col[1];
				m_ChosenMaterial->m_Color.z = col[2];
			}
			else
			{
				col[0] = m_ChosenMaterial->m_Color.r;
				col[1] = m_ChosenMaterial->m_Color.y;
				col[2] = m_ChosenMaterial->m_Color.z;
			}


			ImGui::DragFloat("Shininess: ", &m_ChosenMaterial->m_Shininess, 0.5f, 1.0f, 512.0f);
			ImGui::DragFloat("Normal Strength: ", &m_ChosenMaterial->m_NormalStrength, 0.02f, 0.0f, 1.0f);

			const std::vector<Texture*>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			SPACE();

			// Albedo texture
			ImGui::PushID("Albedo");
			ImGui::Text(("Albedo Texture: " + m_ChosenMaterial->GetAlbedoTexture()->GetFilePath()).c_str());
			ImGui::Text("Choose new albedo texture: ");

			static int chosenAlbedoIndex = 0;
			ImGui::Combo("Textures", &chosenAlbedoIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();
			

			SPACE();

			// Specular texture
			ImGui::PushID("Specular");
			ImGui::Text(("Specular Texture: " + m_ChosenMaterial->GetSpecularTexture()->GetFilePath()).c_str());
			ImGui::Text("Choose new specular texture: ");

			static int chosenSpecularIndex = 0;
			ImGui::Combo("Textures", &chosenSpecularIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();

			SPACE();

			// Normal texture
			ImGui::PushID("Normal");
			ImGui::Text(("Normal Texture: " + m_ChosenMaterial->GetNormalTexture()->GetFilePath()).c_str());
			ImGui::Text("Choose new normal texture: ");

			static int chosenNormalIndex = 0;
			ImGui::Combo("Textures", &chosenNormalIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();
			

			if (ImGui::Button("Save", { 100, 100 }))
			{
				std::string nameStr(name);

				if (nameStr.length() <= 0 || m_AssetManager->ContainsMaterial(nameStr))
					EN_WARN("Enter a valid material name!")
				else
				{
					m_AssetManager->RenameMaterial(m_ChosenMaterial->GetName(), name);

					if (chosenAlbedoIndex == 0)
						m_ChosenMaterial->SetAlbedoTexture(Texture::GetWhiteSRGBTexture());
					else
						m_ChosenMaterial->SetAlbedoTexture(m_AssetManager->GetTexture(allTextures[chosenAlbedoIndex - 1]->GetName()));

					if (chosenSpecularIndex == 0)
						m_ChosenMaterial->SetSpecularTexture(Texture::GetGreyNonSRGBTexture());
					else
						m_ChosenMaterial->SetSpecularTexture(m_AssetManager->GetTexture(allTextures[chosenSpecularIndex - 1]->GetName()));

					if (chosenNormalIndex == 0)
						m_ChosenMaterial->SetNormalTexture(Texture::GetNormalTexture());
					else
						m_ChosenMaterial->SetNormalTexture(m_AssetManager->GetTexture(allTextures[chosenNormalIndex - 1]->GetName()));

					m_IsCreatingMaterial = false;
				}
			}
		}

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