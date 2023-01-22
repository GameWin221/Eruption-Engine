#include "AssetManagerPanel.hpp"

#include <portable-file-dialogs.h>

#include <Common/Helpers.hpp>

namespace en
{
	AssetManagerPanel::AssetManagerPanel(AssetManager* assetManager, EditorImageAtlas* atlas) : m_AssetManager(assetManager), m_Atlas(atlas) {}

	void AssetManagerPanel::Render()
	{
		DrawAssetManager();

		if (IsTypeSelected<Material>() && m_ShowAssetEditor)
			EditingMaterial(&m_ShowAssetEditor);

		else if (IsTypeSelected<Mesh>() && m_ShowAssetEditor)
			EditingMesh(&m_ShowAssetEditor);

		else if (IsTypeSelected<Texture>() && m_ShowAssetEditor)
			EditingTexture(&m_ShowAssetEditor);

		if (m_IsImportingMesh)
			ImportingMesh(&m_IsImportingMesh);

		if (m_IsImportingTexture)
			ImportingTexture(&m_IsImportingTexture);

		if (m_IsCreatingMaterial)
			CreatingMaterial();
	}

	void AssetManagerPanel::DrawAssetManager()
	{
		const ImVec2 buttonSize(100, 100);
		const ImVec2 assetSize(110, 110);
		const float assetsCoverage = 0.85f;

		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);

		ImGui::Begin("Asset Manager", nullptr, EditorCommons::CommonFlags | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		static char buf[64];

		ImGui::BeginChild("Filtering", ImVec2(320, ImGui::GetWindowSize().y * 0.1f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::InputText("Search...", buf, 64);
		ImGui::EndChild();
		int end = 0;

		for (; end < 64; end++)
			if(buf[end] == char(0))
				break;

		std::string searchedName(buf, end);

		std::for_each(searchedName.begin(), searchedName.end(), [](char& c) {c = ::tolower(c); });

		static bool showMesh = true;
		static bool showMaterial = true;
		static bool showTexture = true;

		ImGui::SameLine();
		ImGui::BeginChild("FilteringCheckbox", ImVec2(300, ImGui::GetWindowSize().y * 0.1f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Checkbox("Meshes", &showMesh);
		ImGui::SameLine();
		ImGui::Checkbox("Materials", &showMaterial);
		ImGui::SameLine();
		ImGui::Checkbox("Textures", &showTexture);
		ImGui::EndChild();

		ImGui::BeginChild("AssetButtons", ImVec2(230, ImGui::GetWindowSize().y * 0.86f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		if (ImGui::Button("Import Mesh", buttonSize) && !m_IsCreatingMaterial)
			m_IsImportingMesh = true;

		ImGui::SameLine();

		if (ImGui::Button("Import Texture", buttonSize) && !m_IsCreatingMaterial)
			m_IsImportingTexture = true;

		if (ImGui::Button("Create Material", buttonSize) && !m_IsCreatingMaterial)
			m_IsCreatingMaterial = true;


		ImGui::EndChild();

		ImGui::SameLine();

		const ImVec2 assetPreviewerSize(ImGui::GetWindowSize().x * assetsCoverage - 5, ImGui::GetWindowSize().y * 0.86f);

		ImGui::BeginChild("AssetPreviewer", assetPreviewerSize, true);

		const std::vector<Handle<Mesh>>     meshes	 = m_AssetManager->GetAllMeshes();
		const std::vector<Handle<Texture>>  textures  = m_AssetManager->GetAllTextures();
		const std::vector<Handle<Material>> materials = m_AssetManager->GetAllMaterials();

		std::vector<Asset*> assets;

		assets.reserve(meshes.size() + textures.size() + materials.size() + 1);
		
		if(showMesh)
		for (const auto& mesh : meshes)
		{
			std::string name = mesh->GetName();

			std::for_each(name.begin(), name.end(), [](char& c) {c = std::tolower(c);});

			if (searchedName.size() == 0 || name.find(searchedName) != std::string::npos)
				assets.emplace_back(mesh->CastTo<Asset>());
		}

		if(showTexture)
		for (const auto& texture : textures)
		{
			std::string name = texture->GetName();

			std::for_each(name.begin(), name.end(), [](char& c) {c = ::tolower(c); });

			if (searchedName.size() == 0 || name.find(searchedName) != std::string::npos)
				assets.emplace_back(texture->CastTo<Asset>());
		}

		if(showMaterial)
		for (const auto& material : materials)
		{
			std::string name = material->GetName();

			std::for_each(name.begin(), name.end(), [](char& c) {c = ::tolower(c); });

			if (searchedName.size() == 0 || name.find(searchedName) != std::string::npos)
				assets.emplace_back(material->CastTo<Asset>());
		}

		
		int sameLineAssets = static_cast<int>(ImGui::GetWindowSize().x / (assetSize.x + 20.0f));

		if (sameLineAssets <= 0) sameLineAssets = 1;

		for (int i = 0; i < assets.size(); i += sameLineAssets)
		{
			for (int j = 0; j < sameLineAssets && i + j < assets.size(); j++)
			{
				Asset* asset = assets[i + j];
				
				if (asset->m_Type == AssetType::Mesh)
				{
					Mesh* mesh{ asset->CastTo<Mesh>() };
					const std::string& name{ mesh->GetName() };

					ImGui::PushID((name + "Mesh").c_str());

					if (AssetButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(2, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenAsset = mesh;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}

					ImGui::PopID();
				}
				else if (asset->m_Type == AssetType::Texture)
				{
					Texture* texture{ asset->CastTo<Texture>() };
					const std::string& name{texture->GetName()};

					ImGui::PushID((name + "Texture").c_str());

					if (AssetButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(1, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenAsset = texture;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}

					ImGui::PopID();
				}
				else if (asset->m_Type == AssetType::Material)
				{
					Material* material{ asset->CastTo<Material>() };
					const std::string& name{ material->GetName() };

					ImGui::PushID((name + "Material").c_str());

					if (AssetButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(0, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenAsset = material;
						m_ShowAssetEditor = true;
						m_AssetEditorInit = true;
					}

					ImGui::PopID();
				}
				
				if (j != sameLineAssets - 1)
					ImGui::SameLine();
			}
		}
		
		ImGui::EndChild();

		ImGui::End();
	}

	void AssetManagerPanel::ImportingMesh(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		if (ImGui::Begin("Mesh Importer", open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static MeshImportProperties properties = MeshImportProperties{};

			ImGui::Checkbox("Import Materials", &properties.importMaterials);

			ImGui::Spacing();

			if (properties.importMaterials)
			{
				ImGui::Checkbox("Import Color", &properties.importColor);

				ImGui::Spacing();

				ImGui::Checkbox("Albedo Textures", &properties.importAlbedoTextures);
				ImGui::Checkbox("Roughness Textures", &properties.importRoughnessTextures);
				ImGui::Checkbox("Metalness Textures", &properties.importMetalnessTextures);
				ImGui::Checkbox("Normal Textures", &properties.importNormalTextures);
			}

			SPACE();

			if (ImGui::Button("Select mesh to import...", ImVec2(200, 100)))
			{
				auto file = pfd::open_file("Choose meshes to import...", DEFAULT_ASSET_PATH, { "Supported Mesh Formats", "*.gltf *.fbx *.obj" }, pfd::opt::multiselect);

				const std::vector<std::string> filePaths = file.result();

				for (const auto& path : filePaths)
				{
					std::string fileName = path.substr(path.find_last_of('/') + 1, path.length());
					fileName = path.substr(path.find_last_of('\\') + 1, path.length());

					m_AssetManager->LoadMesh(fileName, path, properties);
				}
			}

			ImGui::End();
		}

		ImGui::PopStyleColor();
	}
	void AssetManagerPanel::ImportingTexture(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		if (ImGui::Begin("Texture Importer", open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static TextureImportProperties properties = TextureImportProperties{};
			
			static int formatIndex = 0;

			static std::vector<const char*> formats = {"Color (sRGB)", "Non-Color (XYZ)"};

			if (ImGui::Combo("Textures", &formatIndex, formats.data(), formats.size()))
			{
				switch (formatIndex)
				{
					case 0:
						properties.format = TextureFormat::Color;
						break;
					case 1:
						properties.format = TextureFormat::NonColor;
						break;
					default:
						EN_ERROR("AssetManagerPanel::ImportingTexture() - Invalid format index!");
						break;
				}
			}

			ImGui::Checkbox("Load Flipped", &properties.flipped);

			if (ImGui::Button("Choose textures to importer...", ImVec2(200, 100)))
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

			ImGui::End();
		}

		ImGui::PopStyleColor();
	}

	void AssetManagerPanel::EditingMaterial(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		Material* chosenMaterial = m_ChosenAsset->CastTo<Material>();

		if (ImGui::Begin(("Material editor - \"" + chosenMaterial->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static char name[86];

			static int chosenAlbedoIndex	= 0;
			static int chosenRoughnessIndex = 0;
			static int chosenNormalIndex	= 0;
			static int chosenMetalnessIndex = 0;

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, chosenMaterial->GetName().c_str());

				const auto& textures = m_AssetManager->GetAllTextures();

				chosenAlbedoIndex	 = 0;
				chosenRoughnessIndex = 0;
				chosenNormalIndex	 = 0;
				chosenMetalnessIndex = 0;

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == chosenMaterial->GetAlbedoTexture()->GetName())
					{
						chosenAlbedoIndex = i + 1;
						break;
					}

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == chosenMaterial->GetRoughnessTexture()->GetName())
					{
						chosenRoughnessIndex = i + 1;
						break;
					}

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == chosenMaterial->GetNormalTexture()->GetName())
					{
						chosenNormalIndex = i + 1;
						break;
					}

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == chosenMaterial->GetMetalnessTexture()->GetName())
					{
						chosenMetalnessIndex = i + 1;
						break;
					}

				m_AssetEditorInit = false;
			}

			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_AssetManager->RenameMaterial(chosenMaterial->GetName(), name);

			glm::vec3 col = chosenMaterial->GetColor();

			ImGui::ColorEdit3("Color: ", (float*)&col);

			col = glm::clamp(col, glm::vec3(0.0f), glm::vec3(1.0f));

			chosenMaterial->SetColor(col);

			const std::vector<Handle<Texture>>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			// Albedo texture
			ImGui::PushID("Albedo");
			ImGui::Text("Albedo texture: ");

			if (ImGui::Combo("Textures", &chosenAlbedoIndex, textureNames.data(), textureNames.size()))
			{
				if (chosenAlbedoIndex == 0)
					chosenMaterial->SetAlbedoTexture(Texture::GetWhiteSRGBTexture());
				else
					chosenMaterial->SetAlbedoTexture(m_AssetManager->GetTexture(allTextures[chosenAlbedoIndex - 1]->GetName()));
			}
			ImGui::PopID();
			
			SPACE();

			// Roughness texture
			ImGui::PushID("Roughness");
			ImGui::Text("Roughness texture: ");

			if (ImGui::Combo("Textures", &chosenRoughnessIndex, textureNames.data(), textureNames.size()))
			{
				if (chosenRoughnessIndex == 0)
					chosenMaterial->SetRoughnessTexture(Texture::GetWhiteNonSRGBTexture());
				else
					chosenMaterial->SetRoughnessTexture(m_AssetManager->GetTexture(allTextures[chosenRoughnessIndex - 1]->GetName()));
			}
			ImGui::PopID();

			float rgh = chosenMaterial->GetRoughness();

			if (chosenRoughnessIndex == 0)
				if (ImGui::DragFloat("Roughness: ", &rgh, 0.01f, 0.0f, 1.0, "%.3f", ImGuiSliderFlags_AlwaysClamp))
					chosenMaterial->SetRoughness(rgh);



			SPACE();

			// Normal texture
			ImGui::PushID("Normal");
			ImGui::Text("Normal texture: ");

			if (ImGui::Combo("Textures", &chosenNormalIndex, textureNames.data(), textureNames.size()))
			{
				if (chosenNormalIndex == 0)
					chosenMaterial->SetNormalTexture(Texture::GetWhiteNonSRGBTexture());
				else
					chosenMaterial->SetNormalTexture(m_AssetManager->GetTexture(allTextures[chosenNormalIndex - 1]->GetName()));
			}
			ImGui::PopID();
			
			float nrm = chosenMaterial->GetNormalStrength();

			if (chosenNormalIndex != 0)
				if (ImGui::DragFloat("Normal Strength: ", &nrm, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
					chosenMaterial->SetNormalStrength(nrm);

			SPACE();

			// Metalness texture
			ImGui::PushID("Metalness");
			ImGui::Text("Metalness texture: ");

			if (ImGui::Combo("Textures", &chosenMetalnessIndex, textureNames.data(), textureNames.size()))
			{
				if (chosenMetalnessIndex == 0)
					chosenMaterial->SetMetalnessTexture(Texture::GetWhiteNonSRGBTexture());
				else
					chosenMaterial->SetMetalnessTexture(m_AssetManager->GetTexture(allTextures[chosenMetalnessIndex - 1]->GetName()));
			}
			ImGui::PopID();

			float mtl = chosenMaterial->GetMetalness();

			if (chosenMetalnessIndex == 0)
				if (ImGui::DragFloat("Metalness: ", &mtl, 0.01f, 0.0f, 1.0, "%.3f", ImGuiSliderFlags_AlwaysClamp))
					chosenMaterial->SetMetalness(mtl);
		}

		ImGui::End();


		ImGui::PopStyleColor();
	}
	void AssetManagerPanel::EditingMesh(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		Mesh* chosenMesh = m_ChosenAsset->CastTo<Mesh>();

		if (ImGui::Begin(("Mesh editor - \"" + chosenMesh->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static char name[86];

			static std::vector<int> chosenMaterialIndices{};

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, chosenMesh->GetName().c_str());

				chosenMaterialIndices.clear();
				chosenMaterialIndices.resize(chosenMesh->m_SubMeshes.size());

				const auto& materials = m_AssetManager->GetAllMaterials();

				for (int i = 0; i < chosenMesh->m_SubMeshes.size(); i++)
				{
					for (int matIndex = 0; matIndex < materials.size(); matIndex++)
						if (materials[matIndex]->GetName() == chosenMesh->m_SubMeshes[i].m_Material->GetName())
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
				m_AssetManager->RenameMesh(chosenMesh->GetName(), name);

			ImGui::Text(("Path: " + chosenMesh->GetFilePath()).c_str());

			SPACE();

			ImGui::Checkbox("Is Active", &chosenMesh->m_Active);

			SPACE();

			for (int id = 0; auto & subMesh : chosenMesh->m_SubMeshes)
			{
				ImGui::PushID(id);

				if (ImGui::CollapsingHeader(("Submesh [" + std::to_string(id) + "]").c_str()))
				{
					ImGui::Text(("Indices: " + std::to_string(subMesh.m_VertexBuffer->m_VerticesCount)).c_str());

					SPACE();

					ImGui::Text("Material: ");
					const std::vector<Handle<Material>>& allMaterials = m_AssetManager->GetAllMaterials();

					std::vector<const char*> materialNames(allMaterials.size() + 1);

					materialNames[0] = "No material";

					for (int i = 1; i < materialNames.size(); i++)
						materialNames[i] = allMaterials[i - 1]->GetName().c_str();

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
	void AssetManagerPanel::EditingTexture(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		Texture* chosenTexture = m_ChosenAsset->CastTo<Texture>();

		if (ImGui::Begin(("Texture editor - \"" + chosenTexture->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static char name[86];

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, chosenTexture->GetName().c_str());
				m_AssetEditorInit = false;
			}

			ImGui::InputText("Name: ", name, 86);

			ImGui::SameLine();

			if (ImGui::Button("Save Name"))
				m_AssetManager->RenameTexture(chosenTexture->GetName(), name);

			ImGui::Text(("Path: " + chosenTexture->GetFilePath()).c_str());
		}

		ImGui::End();
		ImGui::PopStyleColor();
	}
	void AssetManagerPanel::CreatingMaterial()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		Material* material = m_ChosenAsset->CastTo<Material>();

		if (ImGui::Begin("Creating a new material", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { 1.0f, 1.0f, 1.0f };
			static float metalness = 0.0f;
			static float roughness = 0.75f;

			static float normalStrength = 1.0f;

			static char name[86];

			ImGui::InputText("Name: ", name, 86);

			ImGui::ColorEdit3("Color: ", col);

			col[0] = std::fmaxf(col[0], 0.0f);
			col[1] = std::fmaxf(col[1], 0.0f);
			col[2] = std::fmaxf(col[2], 0.0f);

			const std::vector<Handle<Texture>>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			SPACE();

			// Albedo texture
			ImGui::PushID("Albedo");
			ImGui::Text("Choose new albedo texture: ");

			static int chosenAlbedoIndex = 0;
			ImGui::Combo("", &chosenAlbedoIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();


			SPACE();

			// Roughness texture
			ImGui::PushID("Roughness");
			ImGui::Text("Choose new roughness texture: ");

			static int chosenRoughnessIndex = 0;
			ImGui::Combo("", &chosenRoughnessIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();

			if(chosenRoughnessIndex == 0)
				ImGui::DragFloat("Roughness Value", &roughness, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

			SPACE();

			// Normal texture
			ImGui::PushID("Normal");
			ImGui::Text("Choose new normal texture: ");

			static int chosenNormalIndex = 0;
			ImGui::Combo("", &chosenNormalIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();

			if(chosenNormalIndex != 0)
				ImGui::DragFloat("Normal Strength", &normalStrength, 0.02f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

			SPACE();

			// Normal texture
			ImGui::PushID("Metalness");
			ImGui::Text("Choose new metalness texture: ");

			static int chosenMetalnessIndex = 0;
			ImGui::Combo("", &chosenMetalnessIndex, textureNames.data(), textureNames.size());
			ImGui::PopID();

			if(chosenMetalnessIndex == 0)
				ImGui::DragFloat("Metalness Value", &metalness, 0.01f, 0.0f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

			if (ImGui::Button("Save", { 100, 100 }))
			{
				std::string nameStr(name);

				if (nameStr.length() <= 0 || m_AssetManager->ContainsMaterial(nameStr))
					EN_WARN("Enter a valid material name!")
				else
				{
					Handle<Texture> albedoTex    = ((chosenAlbedoIndex    == 0) ? Texture::GetWhiteSRGBTexture()	 : m_AssetManager->GetTexture(allTextures[chosenAlbedoIndex    - 1]->GetName()));
					Handle<Texture> roughnessTex = ((chosenRoughnessIndex == 0) ? Texture::GetWhiteNonSRGBTexture() : m_AssetManager->GetTexture(allTextures[chosenRoughnessIndex - 1]->GetName()));
					Handle<Texture> normalTex    = ((chosenNormalIndex    == 0) ? Texture::GetWhiteNonSRGBTexture() : m_AssetManager->GetTexture(allTextures[chosenNormalIndex    - 1]->GetName()));
					Handle<Texture> metalnessTex = ((chosenMetalnessIndex == 0) ? Texture::GetWhiteNonSRGBTexture() : m_AssetManager->GetTexture(allTextures[chosenMetalnessIndex - 1]->GetName()));

					if (roughnessTex != Texture::GetWhiteNonSRGBTexture())
						roughness = 1.0f;

					if (metalnessTex != Texture::GetWhiteNonSRGBTexture())
						metalness = 1.0f;

					m_AssetManager->CreateMaterial(nameStr, glm::vec3(col[0], col[1], col[2]), metalness, roughness, normalStrength, albedoTex, roughnessTex, normalTex, metalnessTex);

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

	bool AssetManagerPanel::AssetButtonLabeled(const std::string& label, const glm::vec2& size, const glm::uvec2& imagePos)
	{
		const ImageUVs UVs = m_Atlas->GetImageUVs(imagePos.x, imagePos.y);

		ImGui::BeginChild("Material", ImVec2(size.x + 10.0f, size.y + 10.0f), false, ImGuiWindowFlags_NoCollapse);

		const bool pressed = ImGui::ImageButton(m_Atlas->m_DescriptorSet, ImVec2(size.x, size.y), UVs.uv0, UVs.uv1);

		EditorCommons::TextCentered(label);

		ImGui::EndChild();

		return pressed;
	}
}