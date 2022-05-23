#include "Core/EnPch.hpp"
#include "AssetManagerPanel.hpp"

#include <portable-file-dialogs.h>

namespace en
{
	AssetManagerPanel::AssetManagerPanel(AssetManager* assetManager, EditorImageAtlas* atlas) : m_AssetManager(assetManager), m_Atlas(atlas) {}

	void AssetManagerPanel::Render()
	{
		DrawAssetManager();

		if (m_ChosenMaterial && m_ShowAssetEditor)
			EditingMaterial(&m_ShowAssetEditor);

		else if (m_ChosenMesh && m_ShowAssetEditor)
			EditingMesh(&m_ShowAssetEditor);

		else if (m_ChosenTexture && m_ShowAssetEditor)
			EditingTexture(&m_ShowAssetEditor);

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

		ImGui::BeginChild("AssetButtons", ImVec2(ImGui::GetWindowSize().x * (1.0f - assetsCoverage), ImGui::GetWindowSize().y * 0.86f), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

		if (ImGui::Button("Import Mesh", buttonSize) && !m_IsCreatingMaterial)
		{
			auto file = pfd::open_file("Choose meshes to import...", DEFAULT_ASSET_PATH, { "Supported Mesh Formats", "*.gltf *.fbx *.obj" }, pfd::opt::multiselect);

			const std::vector<std::string> filePaths = file.result();

			for (const auto& path : filePaths)
			{
				std::string fileName = path.substr(path.find_last_of('/') + 1, path.length());
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

		std::vector<Mesh*>     meshes = m_AssetManager->GetAllMeshes();
		std::vector<Texture*>  textures = m_AssetManager->GetAllTextures();
		std::vector<Material*> materials = m_AssetManager->GetAllMaterials();

		std::vector<AssetRef> assets;

		for (const auto& mesh : meshes)
			assets.emplace_back(mesh, AssetType::Mesh);

		for (const auto& texture : textures)
			assets.emplace_back(texture, AssetType::Texture);

		for (const auto& material : materials)
			assets.emplace_back(material, AssetType::Material);

		int sameLineAssets = static_cast<int>(ImGui::GetWindowSize().x / (assetSize.x + 20.0f));

		if (sameLineAssets <= 0) sameLineAssets = 1;

		for (int i = 0; i < assets.size(); i += sameLineAssets)
		{
			for (int j = 0; j < sameLineAssets && i + j < assets.size(); j++)
			{
				AssetRef& asset = assets[i + j];

				if (asset.type == AssetType::Mesh)
				{
					Mesh* mesh = asset.CastTo<Mesh*>();
					std::string name = mesh->GetName();

					ImGui::PushID((name + "Mesh").c_str());

					if (AssetButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(2, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = nullptr;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = mesh;
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

					if (AssetButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(1, 0)) && !m_IsCreatingMaterial)
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

					if (AssetButtonLabeled(name, glm::vec2(assetSize.x, assetSize.y), glm::uvec2(0, 0)) && !m_IsCreatingMaterial)
					{
						m_ChosenMaterial = material;
						m_ChosenTexture = nullptr;
						m_ChosenMesh = nullptr;
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

	void AssetManagerPanel::EditingMaterial(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		if (ImGui::Begin(("Material editor - \"" + m_ChosenMaterial->GetName() + "\"").c_str(), open, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { m_ChosenMaterial->m_Color.r, m_ChosenMaterial->m_Color.y, m_ChosenMaterial->m_Color.z };

			static char name[86];

			static int chosenAlbedoIndex = 0;
			static int chosenRoughnessIndex = 0;
			static int chosenNormalIndex = 0;

			if (m_AssetEditorInit)
			{
				strcpy_s(name, sizeof(char) * 86, m_ChosenMaterial->GetName().c_str());

				const auto& textures = m_AssetManager->GetAllTextures();

				chosenAlbedoIndex = 0;
				chosenRoughnessIndex = 0;
				chosenNormalIndex = 0;

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == m_ChosenMaterial->GetAlbedoTexture()->GetName())
					{
						chosenAlbedoIndex = i + 1;
						break;
					}

				for (int i = 0; i < textures.size(); i++)
					if (textures[i]->GetName() == m_ChosenMaterial->GetRoughnessTexture()->GetName())
					{
						chosenRoughnessIndex = i + 1;
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


			ImGui::DragFloat("Metalness: ", &m_ChosenMaterial->m_Metalness, 0.01f, 0.0f, 1.0);
			ImGui::DragFloat("Normal Strength: ", &m_ChosenMaterial->m_NormalStrength, 0.02f, 0.0f, 1.0f);

			const std::vector<Texture*>& allTextures = m_AssetManager->GetAllTextures();

			std::vector<const char*> textureNames(allTextures.size() + 1);
			textureNames[0] = "No texture";

			for (int i = 1; i < textureNames.size(); i++)
				textureNames[i] = allTextures[i - 1]->GetName().c_str();

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

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

			// Roughness texture
			{
				ImGui::PushID("Roughness");
				ImGui::Text("Roughness texture: ");

				if (ImGui::Combo("Textures", &chosenRoughnessIndex, textureNames.data(), textureNames.size()))
				{
					if (chosenRoughnessIndex == 0)
						m_ChosenMaterial->SetRoughnessTexture(Texture::GetGreyNonSRGBTexture());
					else
						m_ChosenMaterial->SetRoughnessTexture(m_AssetManager->GetTexture(allTextures[chosenRoughnessIndex - 1]->GetName()));
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
	void AssetManagerPanel::EditingMesh(bool* open)
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

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

			for (int id = 0; auto & subMesh : m_ChosenMesh->m_SubMeshes)
			{
				ImGui::PushID(id);

				if (ImGui::CollapsingHeader(("Submesh [" + std::to_string(id) + "]").c_str()))
				{
					ImGui::Text(("Indices: " + std::to_string(subMesh.m_VertexBuffer->GetVerticesCount())).c_str());

					SPACE();

					ImGui::Text("Material: ");
					const std::vector<Material*>& allMaterials = m_AssetManager->GetAllMaterials();

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
	void AssetManagerPanel::CreatingMaterial()
	{
		ImGui::SetNextWindowSizeConstraints(EditorCommons::FreeWindowMinSize, EditorCommons::FreeWindowMaxSize);
		ImGui::PushStyleColor(ImGuiCol_WindowBg, m_AssetEditorBG.Value);

		if (ImGui::Begin("Creating a new material", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoSavedSettings))
		{
			static float col[3] = { 1.0f, 1.0f, 1.0f };
			static float metalness = 0.0f;

			static float normalStrength = 1.0f;

			static char name[86];

			ImGui::InputText("Name: ", name, 86);

			ImGui::ColorEdit3("Color: ", col);

			ImGui::DragFloat("Metalness: ", &metalness, 0.5f, 1.0f, 512.0f);
			ImGui::DragFloat("Normal Strength: ", &normalStrength, 0.02f, 0.0f, 1.0f);

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

			// Roughness texture
			ImGui::PushID("Roughness");
			ImGui::Text("Choose new roughness texture: ");

			static int chosenRoughnessIndex = 0;
			ImGui::Combo("Textures", &chosenRoughnessIndex, textureNames.data(), textureNames.size());
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
					Texture* albedo = ((chosenAlbedoIndex == 0) ? Texture::GetWhiteSRGBTexture() : m_AssetManager->GetTexture(allTextures[chosenAlbedoIndex - 1]->GetName()));
					Texture* rougness = ((chosenRoughnessIndex == 0) ? Texture::GetGreyNonSRGBTexture() : m_AssetManager->GetTexture(allTextures[chosenRoughnessIndex - 1]->GetName()));
					Texture* normal = ((chosenNormalIndex == 0) ? Texture::GetNormalTexture() : m_AssetManager->GetTexture(allTextures[chosenNormalIndex - 1]->GetName()));

					m_AssetManager->CreateMaterial(nameStr, glm::vec3(col[0], col[1], col[2]), metalness, normalStrength, albedo, rougness, normal);

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

	bool AssetManagerPanel::AssetButtonLabeled(std::string label, glm::vec2 size, glm::uvec2 imagePos)
	{
		bool pressed = false;

		const ImageUVs UVs = m_Atlas->GetImageUVs(imagePos.x, imagePos.y);

		ImGui::BeginChild("Material", ImVec2(size.x + 10.0f, size.y + 10.0f), false, ImGuiWindowFlags_NoCollapse);

		pressed = ImGui::ImageButton(m_Atlas->m_DescriptorSet, ImVec2(size.x, size.y), UVs.uv0, UVs.uv1);

		EditorCommons::TextCentered(label);

		ImGui::EndChild();

		return pressed;
	}
}