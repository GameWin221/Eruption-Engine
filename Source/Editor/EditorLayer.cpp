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
			if(!renderer)     EN_WARN("-The 'renderer'     value was a nullptr!");
			if(!deltaTimeVar) EN_WARN("-The 'deltaTimeVar' value was a nullptr!");
			if(!assetManager) EN_WARN("-The 'assetManager' value was a nullptr!");
			return;
		}

		m_DeltaTime = deltaTimeVar;
		m_Renderer = renderer;
		m_AssetManager = assetManager;

		m_Atlas = std::make_unique<EditorImageAtlas>("Models/Atlas.png", 4, 1);

		m_Renderer->SetUIRenderCallback(std::bind(&EditorLayer::OnUIDraw, this));

		m_CommonFlags = ImGuiWindowFlags_None | ImGuiWindowFlags_NoCollapse;
		m_DockedWindowBG	= ImColor(40, 40, 40, 100);
		m_FreeWindowBG		= ImColor(40, 40, 40, 255);
		m_FreeWindowMinSize = ImVec2 (500 , 250);
		m_FreeWindowMaxSize = ImVec2 (1920, 1080);

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

		if (m_ShowAssetMenu)
			DrawAssetMenu();

		if (m_ShowDebugMenu)
			DrawDebugMenu();

		if (m_ShowSceneMenu)
			DrawSceneMenu();

		if (m_ShowInspector)
			DrawInspector();

		// Asset Editors
		if (m_ChosenMaterial && m_ShowAssetEditor)
			EditingMaterial(&m_ShowAssetEditor);
		
		else if (m_ChosenMesh && m_ShowAssetEditor)
			EditingMesh(&m_ShowAssetEditor);

		else if (m_ChosenTexture && m_ShowAssetEditor)
			EditingTexture(&m_ShowAssetEditor);

		if (m_IsCreatingMaterial)
			CreatingMaterial();

		EndRender();
	}

	void EditorLayer::TextCentered(std::string text)
	{
		float fontSize = ImGui::GetFontSize() * text.size() / 2;

		ImGui::SameLine(ImGui::GetWindowSize().x / 2 - fontSize + (fontSize / 2));

		ImGui::Text(text.c_str());
	}

	bool EditorLayer::ShowImageButtonLabeled(std::string label, glm::vec2 size, glm::uvec2 imagePos)
	{
		bool pressed = false;

		const ImageUVs UVs = m_Atlas->GetImageUVs(imagePos.x, imagePos.y);

		ImGui::BeginChild("Material", ImVec2(size.x + 10.0f, size.y+10.0f), false, ImGuiWindowFlags_NoCollapse);

		pressed = ImGui::ImageButton(m_Atlas->m_DescriptorSet, ImVec2(size.x, size.y), UVs.uv0, UVs.uv1);

		TextCentered(label);

		ImGui::EndChild();

		return pressed;
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
			ImGui::MenuItem("Asset Menu" , "", &m_ShowAssetMenu);
			ImGui::MenuItem("Camera Menu", "", &m_ShowCameraMenu);
			ImGui::MenuItem("Lights Menu", "", &m_ShowLightsMenu);
			ImGui::MenuItem("Debug Menu" , "", &m_ShowDebugMenu);

			ImGui::MenuItem("Scene Menu", "", &m_ShowSceneMenu);
			ImGui::MenuItem("Inspector" , "", &m_ShowInspector);
		
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
		const ImVec2 assetSize(110, 110);
		const float assetsCoverage = 0.85f;
		
		ImGui::Begin("Asset Manager", nullptr, m_CommonFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		ImGui::BeginChild("AssetButtons", ImVec2(ImGui::GetWindowSize().x * (1.0f - assetsCoverage), ImGui::GetWindowSize().y * 0.86f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		
		if (ImGui::Button("Import Mesh", buttonSize) && !m_IsCreatingMaterial)
		{
			auto file = pfd::open_file("Choose meshes to import...", DEFAULT_ASSET_PATH, { "Supported Mesh Formats", "*.gltf *.fbx *.obj" }, pfd::opt::multiselect);

			const std::vector<std::string> filePaths = file.result();

			for (const auto& path : filePaths)
			{
				std::string fileName = path.substr(path.find_last_of('/') + 1 , path.length());
				fileName = path.substr(path.find_last_of('\\') + 1, path.length());

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
				fileName = path.substr(path.find_last_of('\\') + 1, path.length());

				m_AssetManager->LoadTexture(fileName, path);
			}
		}

		if (ImGui::Button("Create Material", buttonSize) && !m_IsCreatingMaterial)
			m_IsCreatingMaterial = true;
		

		ImGui::EndChild();

		ImGui::SameLine();

		const ImVec2 assetPreviewerSize(ImGui::GetWindowSize().x * (assetsCoverage - 0.015f), ImGui::GetWindowSize().y * 0.86f);

		ImGui::BeginChild("AssetPreviewer", assetPreviewerSize, true);
		
		std::vector<Mesh*>     meshes    = m_AssetManager->GetAllMeshes();
		std::vector<Texture*>  textures  = m_AssetManager->GetAllTextures();
		std::vector<Material*> materials = m_AssetManager->GetAllMaterials();
		
		std::vector<AssetRef> assets;

		for (const auto& mesh : meshes)
			assets.emplace_back(mesh, AssetType::Mesh);

		for (const auto& texture : textures)
			assets.emplace_back(texture, AssetType::Texture);

		for (const auto& material : materials)
			assets.emplace_back(material, AssetType::Material);
		
		int sameLineAssets = static_cast<int>(ImGui::GetWindowSize().x / (assetSize.x+20.0f));

		if (sameLineAssets <= 0) sameLineAssets = 1;

		for (int i = 0; i < assets.size(); i += sameLineAssets)
		{
			for (int j = 0; j < sameLineAssets && i+j < assets.size(); j++)
			{
				AssetRef& asset = assets[i + j];

				if (asset.type == AssetType::Mesh)
				{
					Mesh* mesh = asset.CastTo<Mesh*>();
					std::string name = mesh->GetName();

					ImGui::PushID((name + "Mesh").c_str());

					if (ShowImageButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(2, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = nullptr;
						m_ChosenMesh=mesh;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}

					ImGui::PopID();
				}
				else if (asset.type == AssetType::Texture)
				{
					Texture* texture = asset.CastTo<Texture*>();
					std::string name = texture->GetName();

					ImGui::PushID((name + "Texture").c_str());

					if (ShowImageButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(1, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = texture;
						m_ChosenMesh = nullptr;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}

					ImGui::PopID();
				}
				else if (asset.type == AssetType::Material)
				{
					Material* material = asset.CastTo<Material*>();
					std::string name = material->GetName();

					ImGui::PushID((name + "Material").c_str());

					if (ShowImageButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(0, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = material;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = nullptr;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}

					ImGui::PopID();
				}
			
				if(j!=sameLineAssets-1)
					ImGui::SameLine();
			}
		}
		
		ImGui::EndChild();

		ImGui::End();
	}
	void EditorLayer::DrawSceneMenu()
	{
		ImGui::Begin("Scene Menu");

		if (ImGui::CollapsingHeader("Scene Properties"))
		{
			static float col[3];

			col[0] = m_Renderer->GetScene()->m_AmbientColor.r;
			col[1] = m_Renderer->GetScene()->m_AmbientColor.g;
			col[2] = m_Renderer->GetScene()->m_AmbientColor.b;

			ImGui::Text("Ambient Color:");
			if (ImGui::DragFloat3("", col, 0.05, 0.0, 1.0))
				m_Renderer->GetScene()->m_AmbientColor = glm::vec3(col[0], col[1], col[2]);
		}

		SPACE();

		if (ImGui::Button("Add a SceneObject", ImVec2(ImGui::GetWindowWidth() - 15.0f, 40)))
			m_Renderer->GetScene()->CreateSceneObject("New SceneObject", Mesh::GetEmptyMesh());

		SPACE();

		const std::vector<SceneObject*> sceneObjects = m_Renderer->GetScene()->GetAllSceneObjects();
		const uint32_t sceneObjectCount = sceneObjects.size();

		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.23f, 0.53f, 0.89f, 0.12f));
		for (const auto& object : sceneObjects)
		{
			if (ImGui::Button(object->GetName().c_str(), ImVec2(ImGui::GetWindowWidth() - 15.0f, 20)))
			{
				EN_LOG("Selected " + object->GetName());
				m_ChosenObject = m_Renderer->GetScene()->GetSceneObject(object->GetName());
				m_InspectorInit = true;
			}
			
			ImGui::Spacing();
		}

		ImGui::PopStyleColor();
		ImGui::End();
	}
	void EditorLayer::DrawInspector()
	{
		ImGui::Begin("Inspector");

		if (m_ChosenObject)
		{
			static char name[86];

			static float pos[3];
			static float rot[3];
			static float scl[3];

			static int chosenMeshIndex = 0;

			if (m_InspectorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenObject->GetName().c_str());

				chosenMeshIndex = 0;

				const auto& meshes = m_AssetManager->GetAllMeshes();

				for (int i = 0; i < meshes.size(); i++)
					if (meshes[i]->GetName() == m_ChosenObject->m_Mesh->GetName())
					{
						chosenMeshIndex = i + 1;
						break;
					}

				pos[0] = m_ChosenObject->m_Position.x;
				pos[1] = m_ChosenObject->m_Position.y;
				pos[2] = m_ChosenObject->m_Position.z;

				rot[0] = m_ChosenObject->m_Rotation.x;
				rot[1] = m_ChosenObject->m_Rotation.y;
				rot[2] = m_ChosenObject->m_Rotation.z;

				scl[0] = m_ChosenObject->m_Scale.x;
				scl[1] = m_ChosenObject->m_Scale.y;
				scl[2] = m_ChosenObject->m_Scale.z;

				m_InspectorInit = false;
			}

			if (ImGui::CollapsingHeader("Properties", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::InputText("Name: ", name, 86);

				ImGui::SameLine();

				if (ImGui::Button("Save Name"))
					m_Renderer->GetScene()->RenameSceneObject(m_ChosenObject->GetName(), name);

				SPACE();

				if (ImGui::Button("Delete"))
					m_Renderer->GetScene()->DeleteSceneObject(m_ChosenObject->GetName());

				SPACE();

				ImGui::DragFloat3("Position", pos, 0.1f);
				ImGui::DragFloat3("Rotation", rot, 0.1f);
				ImGui::DragFloat3("Scale", scl, 0.1f);
				ImGui::Checkbox("Active", &m_ChosenObject->m_Active);

				m_ChosenObject->m_Position = glm::vec3(pos[0], pos[1], pos[2]);
				m_ChosenObject->m_Rotation = glm::vec3(rot[0], rot[1], rot[2]);
				m_ChosenObject->m_Scale = glm::vec3(scl[0], scl[1], scl[2]);

				SPACE();

				const std::vector<Mesh*>& allMeshes = m_AssetManager->GetAllMeshes();

				std::vector<const char*> meshNames(allMeshes.size() + 1);

				meshNames[0] = "No mesh";

				for (int i = 1; i < meshNames.size(); i++)
					meshNames[i] = allMeshes[i - 1]->GetName().c_str();

				if (ImGui::Combo("Mesh", &chosenMeshIndex, meshNames.data(), meshNames.size()))
				{
					if (chosenMeshIndex == 0)
						m_ChosenObject->m_Mesh = Mesh::GetEmptyMesh();
					else
						m_ChosenObject->m_Mesh = m_AssetManager->GetMesh(allMeshes[chosenMeshIndex - 1]->GetName());
				}
			}
		}

		else
			ImGui::Text("No SceneObject is chosen!");

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

		static bool vSync = true;
		if (ImGui::Checkbox("VSync", &vSync))
			m_Renderer->SetVSync(vSync);

		ImGui::End();
	}
	void EditorLayer::EndRender()
	{
		ImGui::Render();
	}

	void EditorLayer::EditingMaterial(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(m_FreeWindowMinSize, m_FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin(("Material editor - \"" + m_ChosenMaterial->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { m_ChosenMaterial->m_Color.r, m_ChosenMaterial->m_Color.y, m_ChosenMaterial->m_Color.z};

			static char name[86];

			static int chosenAlbedoIndex = 0;
			static int chosenSpecularIndex = 0;
			static int chosenNormalIndex = 0;

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenMaterial->GetName().c_str());

				const auto& textures = m_AssetManager->GetAllTextures();

				chosenAlbedoIndex = 0;
				chosenSpecularIndex = 0;
				chosenNormalIndex = 0;

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == m_ChosenMaterial->GetAlbedoTexture()->GetName())
					{
						chosenAlbedoIndex = i + 1;
						break;
					}

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == m_ChosenMaterial->GetSpecularTexture()->GetName())
					{
						chosenSpecularIndex = i + 1;
						break;
					}

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == m_ChosenMaterial->GetNormalTexture()->GetName())
					{
						chosenNormalIndex = i + 1;
						break;
					}

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
			ImGui::DragFloat("Specular Strength: ", &m_ChosenMaterial->m_SpecularStrength, 0.02f, 0.0f, 1.0f);

			const std::vector<Texture*>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			SPACE();

			// Albedo texture
			{ 
				ImGui::PushID("Albedo");
				ImGui::Text("Albedo texture: ");

				if (ImGui::Combo("Textures", &chosenAlbedoIndex, textureNames.data(), textureNames.size()))
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
				ImGui::Text("Specular texture: ");

				if (ImGui::Combo("Textures", &chosenSpecularIndex, textureNames.data(), textureNames.size()))
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
				ImGui::Text("Normal texture: ");

				if (ImGui::Combo("Textures", &chosenNormalIndex, textureNames.data(), textureNames.size()))
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
	void EditorLayer::EditingMesh(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(m_FreeWindowMinSize, m_FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin(("Mesh editor - \"" + m_ChosenMesh->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static char name[86];

			static std::vector<int> chosenMaterialIndices{};

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenMesh->GetName().c_str());

				chosenMaterialIndices.clear();
				chosenMaterialIndices.resize(m_ChosenMesh->m_SubMeshes.size());

				const auto& materials = m_AssetManager->GetAllMaterials();

				for (int i = 0; i < m_ChosenMesh->m_SubMeshes.size(); i++)
				{
					for (int matIndex = 0; matIndex < materials.size(); matIndex++)
						if (materials[matIndex]->GetName() == m_ChosenMesh->m_SubMeshes[i].m_Material->GetName())
						{
							chosenMaterialIndices[i] = matIndex + 1;
							break;
						}
				}

				m_AssetEditorInit = false;
			}
			
			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_AssetManager->RenameMesh(m_ChosenMesh->GetName(), name);

			ImGui::Text(("Path: " + m_ChosenMesh->GetFilePath()).c_str());

			SPACE();

			ImGui::Checkbox("Is Active", &m_ChosenMesh->m_Active);

			SPACE();

			for (int id = 0; auto& subMesh : m_ChosenMesh->m_SubMeshes)
			{
				ImGui::PushID(id);

				if (ImGui::CollapsingHeader(("Submesh [" + std::to_string(id) + "]").c_str()))
				{
					ImGui::Text(("Indices: " + std::to_string(subMesh.m_VertexBuffer->GetVerticesCount())).c_str());

					SPACE();

					ImGui::Text("Material: ");
					const std::vector<Material*>& allMaterials = m_AssetManager->GetAllMaterials();

					std::vector<const char*> materialNames(allMaterials.size()+1);

					materialNames[0] = "No material";

					for (int i = 1; i < materialNames.size(); i++)
						materialNames[i] = allMaterials[i-1]->GetName().c_str();

					if (ImGui::Combo("Materials", &chosenMaterialIndices[id], materialNames.data(), materialNames.size()))
					{
						if (chosenMaterialIndices[id] == 0)
							subMesh.m_Material = Material::GetDefaultMaterial();
						else
							subMesh.m_Material = m_AssetManager->GetMaterial(allMaterials[chosenMaterialIndices[id] - 1]->GetName());
					}

					SPACE();

					ImGui::Checkbox("Is Active", &subMesh.m_Active);

					SPACE();
				}
			
				ImGui::PopID();

				id++;
			}
		}

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void EditorLayer::EditingTexture(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(m_FreeWindowMinSize, m_FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin(("Texture editor - \"" + m_ChosenTexture->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static char name[86];

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenTexture->GetName().c_str());
				m_AssetEditorInit = false;
			}

			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_AssetManager->RenameTexture(m_ChosenTexture->GetName(), name);

			ImGui::Text(("Path: " + m_ChosenTexture->GetFilePath()).c_str());
		}

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void EditorLayer::CreatingMaterial()
	{
		ImGui::SetNextWindowSizeConstraints(m_FreeWindowMinSize, m_FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_FreeWindowBG.Value);

		if (ImGui::Begin("Creating a new material", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { 1.0f, 1.0f, 1.0f };
			static float shininess = 48.0f;

			static float normalStrength = 1.0f;
			static float specularStrength = 1.0f;

			static char name[86];

			ImGui::InputText("Name: ", name, 86);
			
			ImGui::ColorEdit3("Color: ", col);

			ImGui::DragFloat("Shininess: ", &shininess, 0.5f, 1.0f, 512.0f);
			ImGui::DragFloat("Normal Strength: ", &normalStrength, 0.02f, 0.0f, 1.0f);
			ImGui::DragFloat("Specular Strength: ", &specularStrength, 0.02f, 0.0f, 1.0f);

			const std::vector<Texture*>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			SPACE();

			// Albedo texture
			ImGui::PushID("Albedo");
			ImGui::Text("Choose new albedo texture: ");

			static int chosenAlbedoIndex = 0;
			ImGui::Combo("Textures", &chosenAlbedoIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();
			

			SPACE();

			// Specular texture
			ImGui::PushID("Specular");
			ImGui::Text("Choose new specular texture: ");

			static int chosenSpecularIndex = 0;
			ImGui::Combo("Textures", &chosenSpecularIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();

			SPACE();

			// Normal texture
			ImGui::PushID("Normal");
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
					Texture* albedo   = ((chosenAlbedoIndex   == 0) ? Texture::GetWhiteSRGBTexture()   : m_AssetManager->GetTexture(allTextures[chosenAlbedoIndex   - 1]->GetName()));
					Texture* specular = ((chosenSpecularIndex == 0) ? Texture::GetGreyNonSRGBTexture() : m_AssetManager->GetTexture(allTextures[chosenSpecularIndex - 1]->GetName()));
					Texture* normal	  = ((chosenNormalIndex   == 0) ? Texture::GetNormalTexture()      : m_AssetManager->GetTexture(allTextures[chosenNormalIndex   - 1]->GetName()));

					m_AssetManager->CreateMaterial(nameStr, glm::vec3(col[0], col[1], col[2]), shininess, normalStrength, specularStrength, albedo, specular, normal);

					m_MatCounter++;

					m_IsCreatingMaterial = false;
				}
			}
			ImGui::SameLine();

			if (ImGui::Button("Cancel", { 100, 100 }))
				m_IsCreatingMaterial = false;
		}
		
		ImGui::End();


		ImGui::PopStyleColor();
	}
}