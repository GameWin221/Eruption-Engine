#include <Core/EnPch.hpp>
#include "AssetManager.hpp"

namespace en
{
    uint32_t g_MatIndex = 0U;

    AssetManager* g_AssetManagerInstance;

    AssetManager::AssetManager()
    {
        if (g_AssetManagerInstance)
            EN_ERROR("Failed to create asset manager because there already is one created!");

        EN_SUCCESS("Created the asset manager");
        g_AssetManagerInstance = this;
    }
    AssetManager::~AssetManager()
    {
        g_AssetManagerInstance = nullptr;
    }
    AssetManager* AssetManager::Get()
    {
        return g_AssetManagerInstance;
    }

    bool AssetManager::LoadMesh(std::string nameID, std::string path, MeshImportProperties properties)
    {
        if (m_Meshes.contains(nameID))
        {
            EN_WARN("AssetManager::LoadMesh() - Failed to load a mesh with name \"" + nameID + "\" from \"" + path + "\" because a model with that name already exists!");
            return false;
        }

        m_Meshes[nameID] = LoadMeshFromFile(path, nameID, properties);
        return true;
    }
    bool AssetManager::LoadTexture(std::string nameID, std::string path, TextureImportProperties properties)
    {
        if (m_Textures.contains(nameID))
        {
            EN_WARN("AssetManager::LoadTexture() - Failed to load a texture with name \"" + nameID + "\" from \"" + path + "\" because a texture with that name already exists!");
            return false;
        }

        m_Textures[nameID] = std::make_unique<Texture>(path, nameID, static_cast<VkFormat>(properties.format), properties.flipped);
        return true;
    }

    void AssetManager::DeleteMesh(std::string nameID)
    {
        EN_WARN("AssetManager::DeleteMesh() - This feature is disabled for now!");
    }
    void AssetManager::DeleteTexture(std::string nameID)
    {
        EN_WARN("AssetManager::DeleteTexture() - This feature is disabled for now!");
    }

    bool AssetManager::CreateMaterial(std::string nameID, glm::vec3 color, float metalnessVal, float roughnessVal, float normalStrength, Texture* albedoTexture, Texture* roughnessTexture, Texture* normalTexture, Texture* metalnessTexture)
    {
        if (m_Materials.contains(nameID))
        {
            EN_WARN("AssetManager::CreateMaterial() - Failed to create a material with name \"" + nameID + "\" because a material with that name already exists!");
            return false;
        }

        m_Materials[nameID] = std::make_unique<Material>(nameID, color, metalnessVal, roughnessVal, normalStrength, albedoTexture, roughnessTexture, normalTexture, metalnessTexture);

        EN_LOG("Created a material (Name: \"" + nameID + "\", Color: "           + std::to_string(color.x) + ", " + std::to_string(color.y) + ", " + std::to_string(color.z) + 
                                                           ", MetalnessVal: "    + std::to_string(metalnessVal)      +
                                                           ", RoughlnessVal: "   + std::to_string(roughnessVal)      +
                                                           ", Normal Strength: " + std::to_string(normalStrength) + 
                                                           ", Albedo: "          + ((albedoTexture)    ? albedoTexture   ->m_Name : "No Texture") +
                                                           ", Roughness: "       + ((roughnessTexture) ? roughnessTexture->m_Name : "No Texture") +
                                                           ", Metalness: "       + ((metalnessTexture) ? metalnessTexture->m_Name : "No Texture") +
                                                           ", Normal: "          + ((normalTexture)    ?  normalTexture  ->m_Name : "No Texture"));
        return true;
    }
    void AssetManager::DeleteMaterial(std::string nameID)
    {
        EN_WARN("AssetManager::DeleteMaterial() - This feature is disabled for now!");
    }

    void AssetManager::RenameMesh(std::string oldNameID, std::string newNameID)
    {
        if (m_Meshes.contains(newNameID))
            EN_WARN("Failed to rename " + oldNameID + " because a mesh with a name " + newNameID + " already exists.")
        else
        {
            if (m_Meshes.contains(oldNameID))
            {
                m_Meshes[newNameID].swap(m_Meshes.at(oldNameID));
                m_Meshes.at(newNameID)->m_Name = newNameID;
                m_Meshes.erase(oldNameID);
            }
            else
                EN_WARN("Failed to rename " + oldNameID + " because a mesh with that name doesn't exist.")
        }
    }
    void AssetManager::RenameTexture(std::string oldNameID, std::string newNameID)
    {
        if (m_Textures.contains(newNameID))
            EN_WARN("Failed to rename " + oldNameID + " because a texture with a name " + newNameID + " already exists.")
        else
        {
            if (m_Textures.contains(oldNameID))
            {
                m_Textures[newNameID].swap(m_Textures.at(oldNameID));
                m_Textures.at(newNameID)->m_Name = newNameID;
                m_Textures.erase(oldNameID);
            }
            else
                EN_WARN("Failed to rename " + oldNameID + " because a texture with that name doesn't exist.")
        }
    }
    void AssetManager::RenameMaterial(std::string oldNameID, std::string newNameID)
    {
        if (m_Materials.contains(newNameID))
            EN_WARN("Failed to rename " + oldNameID + " because a material with a name " + newNameID + " already exists.")
        else
        {
            if (m_Materials.contains(oldNameID))
            {
                m_Materials[newNameID].swap(m_Materials.at(oldNameID));
                m_Materials.at(newNameID)->m_Name = newNameID;
                m_Materials.erase(oldNameID);
            }
            else
                EN_WARN("Failed to rename " + oldNameID + " because a material with that name doesn't exist.")
        }
    }

    void AssetManager::UpdateAssets()
    {
        UpdateMaterials();
    }

    Mesh* AssetManager::GetMesh(std::string nameID)
    {
        // If there's no `nameID` model:
        if (!m_Meshes.contains(nameID))
        {
            EN_WARN("AssetManager::GetModel() - There's currently no loaded mesh named \"" + nameID + "\"!");

            return Mesh::GetEmptyMesh();
        }

        return m_Meshes.at(nameID).get();
    }
    Texture* en::AssetManager::GetTexture(std::string nameID)
    {
        // If there's no `nameID` texture:
        if (!m_Textures.contains(nameID))
        {
            EN_WARN("AssetManager::GetTexture() - There's currently no loaded texture named \"" + nameID + "\"!");
            return Texture::GetWhiteNonSRGBTexture();
        }

        return m_Textures.at(nameID).get();
    }
    Material* AssetManager::GetMaterial(std::string nameID)
    {
        // If there's no `nameID` material:
        if (!m_Materials.contains(nameID))
        {
            EN_WARN("AssetManager::GetMaterial() - There's currently no material named \"" + nameID + "\"!");
            return Material::GetDefaultMaterial();
        }

        return m_Materials.at(nameID).get();
    }

    std::vector<Mesh*> AssetManager::GetAllMeshes()
    {
        std::vector<Mesh*> meshes(m_Meshes.size());

        for (int i = 0; const auto & meshPair : m_Meshes)
            meshes[i++] = meshPair.second.get();
        
        return meshes;
    }
    std::vector<Texture*> AssetManager::GetAllTextures()
    {
        std::vector< Texture*> textures(m_Textures.size());

        for (int i = 0; const auto& texturePair : m_Textures)
            textures[i++] = texturePair.second.get();

        return textures;
    }
    std::vector<Material*> AssetManager::GetAllMaterials()
    {
        std::vector< Material*> materials(m_Materials.size());

        for (int i = 0; const auto& materialPair : m_Materials)
            materials[i++] = materialPair.second.get();

        return materials;
    }

    void AssetManager::UpdateMaterials()
    {
        for (const auto& material : m_Materials)
            material.second->UpdateDescriptorSet();
    }

    std::unique_ptr<Mesh> AssetManager::LoadMeshFromFile(std::string& filePath, std::string& name, MeshImportProperties& importProperties)
    {
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
        mesh->m_FilePath = filePath;
        mesh->m_Name = name;
        
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(filePath,
            aiProcess_CalcTangentSpace         |
            aiProcess_GenSmoothNormals         |
            aiProcess_JoinIdenticalVertices    |
            aiProcess_ImproveCacheLocality     |
            aiProcess_RemoveRedundantMaterials |
            aiProcess_Triangulate              |
            aiProcess_GenUVCoords              |
            aiProcess_SplitLargeMeshes         |
            aiProcess_FindDegenerates          |
            aiProcess_FindInvalidData          |
            aiProcess_FlipUVs
        );

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
            EN_ERROR("AssetManager::LoadMeshFromFile() - " + std::string(importer.GetErrorString()));

        std::string directory = mesh->m_FilePath.substr(0, std::min(mesh->m_FilePath.find_last_of('\\'), mesh->m_FilePath.find_last_of('/'))+1);
        std::vector<Material*> materials;

        if(importProperties.importMaterials)
        for (int i = 0; i < scene->mNumMaterials; i++)
        {
            aiMaterial* material = scene->mMaterials[i];
            aiString name = material->GetName();
            
            if (name.length == 0)
            {
                g_MatIndex++;
                name = aiString("New Material (" + std::to_string(g_MatIndex) + ")");
            }

            if (ContainsMaterial(name.C_Str()))
            {
                materials.emplace_back(GetMaterial(name.C_Str()));
                continue;
            }

            aiString diffusePath;
            material->GetTexture(aiTextureType_DIFFUSE, 0, &diffusePath);
            bool hasDiffuseTex = (diffusePath.length > 0 && importProperties.importAlbedoTextures);

            aiString roughnessPath;
            material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &roughnessPath);
            bool hasRoughnessTex = (roughnessPath.length > 0 && importProperties.importRoughnessTextures);
  
            aiString metallicPath;
            material->GetTexture(aiTextureType_METALNESS, 0, &metallicPath);
            bool hasMetallicTex = (metallicPath.length > 0 && importProperties.importMetalnessTextures);

            aiString normalPath;
            material->GetTexture(aiTextureType_NORMALS, 0, &normalPath);
            bool hasNormalTex = (normalPath.length > 0 && importProperties.importNormalTextures);

            aiColor3D color;
            material->Get(AI_MATKEY_BASE_COLOR, color);
            bool hasColor = (color.r > 0.0f && color.g > 0.0f && color.b > 0.0f);
            if (!hasColor || !importProperties.importColor) color = aiColor3D(1.0f);

            float metalness = 0.0f;
            material->Get(AI_MATKEY_METALLIC_FACTOR, metalness);

            float roughness = 0.0f;
            material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness);

            if(hasDiffuseTex  ) LoadTexture(std::string(diffusePath  .C_Str()), directory + std::string(diffusePath .C_Str()) , { TextureFormat::Color    });
            if(hasRoughnessTex) LoadTexture(std::string(roughnessPath.C_Str()), directory + std::string(roughnessPath.C_Str()), { TextureFormat::NonColor });
            if(hasNormalTex   ) LoadTexture(std::string(normalPath   .C_Str()), directory + std::string(normalPath  .C_Str()) , { TextureFormat::NonColor });
            if(hasMetallicTex ) LoadTexture(std::string(metallicPath .C_Str()), directory + std::string(metallicPath.C_Str()) , { TextureFormat::NonColor });

            Texture* diffuseTexture   = (hasDiffuseTex  ) ? GetTexture(std::string(diffusePath.C_Str()  )) : Texture::GetWhiteSRGBTexture();
            Texture* roughnessTexture = (hasRoughnessTex) ? GetTexture(std::string(roughnessPath.C_Str())) : Texture::GetWhiteNonSRGBTexture();
            Texture* normalTexture    = (hasNormalTex   ) ? GetTexture(std::string(normalPath.C_Str()   )) : Texture::GetWhiteNonSRGBTexture();
            Texture* metalnessTexture = (hasMetallicTex ) ? GetTexture(std::string(metallicPath.C_Str() )) : Texture::GetWhiteNonSRGBTexture();

            CreateMaterial(name.C_Str(), glm::vec3(color.r, color.g, color.b), metalness, roughness, 1.0f, diffuseTexture, roughnessTexture, normalTexture, metalnessTexture);

            materials.emplace_back(GetMaterial(name.C_Str()));
        }

        ProcessNode(scene->mRootNode, scene, materials, mesh.get());
        
        EN_SUCCESS("Succesfully loaded a mesh from \"" + filePath + "\"");

        return mesh;
    }
    void AssetManager::ProcessNode(aiNode* node, const aiScene* scene, std::vector<Material*> materials, Mesh* mesh)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
            Material* material;

            aiVector3D pos(0.0f);
            aiQuaternion rot;

            node->mTransformation.DecomposeNoScaling(rot, pos);

            if (assimpMesh->mMaterialIndex < materials.size() && materials.size() > 0)
                material = materials[assimpMesh->mMaterialIndex];
            else
                material = Material::GetDefaultMaterial();
            
            std::vector<Vertex> vertices;
            std::vector<uint32_t> indices;

            GetVertices(assimpMesh, vertices);
            GetIndices(assimpMesh, indices);

            for (auto& vert : vertices)
            {
                vert.pos.x += pos.x;
                vert.pos.y += pos.y;
                vert.pos.z += pos.z;
            }

            if (vertices.size() > 0 && indices.size() > 0)
                mesh->m_SubMeshes.emplace_back(vertices, indices, material);
        }
        for (unsigned int i = 0; i < node->mNumChildren; i++)
            ProcessNode(node->mChildren[i], scene, materials, mesh);
    }
    void AssetManager::GetVertices(aiMesh* mesh, std::vector<Vertex>& vertices)
    {
        if (!mesh->HasPositions() || !mesh->HasNormals())
        {
            std::string name{ mesh->mName.C_Str() };
            EN_WARN("Failed to get vertices data from \"" + name + "\"!");
            return;
        }

        if(mesh->HasTextureCoords(0))
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
                vertices.emplace_back
                (
                    glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
                    glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),
                    glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z),
                    glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y)
                );
        else
            for (unsigned int i = 0; i < mesh->mNumVertices; i++)
                vertices.emplace_back
                (
                    glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z),
                    glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z),
                    glm::vec3(0),
                    glm::vec2(0)
                );
    }
    void AssetManager::GetIndices(aiMesh* mesh, std::vector<uint32_t>& indices)
    {
        for (unsigned int i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];

            for (unsigned int j = 0; j < face.mNumIndices; j++)
                indices.emplace_back(face.mIndices[j]);
        }
    }
}