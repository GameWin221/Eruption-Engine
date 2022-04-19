#include <Core/EnPch.hpp>
#include "AssetManager.hpp"

namespace en
{
    void AssetManager::LoadMesh(std::string nameID, std::string path)
    {
        if (m_Meshes.contains(nameID))
        {
            EN_WARN("AssetManager::LoadModel() - Failed to load a mesh with name \"" + nameID + "\" because a model with that name already exists!");
            return;
        }

        m_Meshes[nameID] = std::make_unique<Mesh>(path);
    }
    void AssetManager::LoadTexture(std::string nameID, std::string path, bool loadFlipped)
    {
        if (m_Textures.contains(nameID))
        {
            EN_WARN("AssetManager::LoadTexture() - Failed to load a texture with name \"" + nameID + "\" because a texture with that name already exists!");
            return;
        }

        m_Textures[nameID] = std::make_unique<Texture>(path, loadFlipped);
    }

    void AssetManager::UnloadMesh(std::string nameID)
    {
        m_Meshes.erase(nameID);
    }
    void AssetManager::UnloadTexture(std::string nameID)
    {
        m_Textures.erase(nameID);
    }

    void AssetManager::CreateMaterial(std::string nameID, glm::vec3 color, float shininess, Texture* albedoTexture, Texture* specularTexture)
    {
        if (m_Materials.contains(nameID))
        {
            EN_WARN("AssetManager::CreateMaterial() - Failed to create a material with name \"" + nameID + "\" because a material with that name already exists!");
            return;
        }

        m_Materials[nameID] = std::make_unique<Material>(color, shininess, albedoTexture, specularTexture);
    }
    void AssetManager::DeleteMaterial(std::string nameID)
    {
        m_Materials.erase(nameID);
    }

    Mesh* AssetManager::GetMesh(std::string nameID)
    {
        // If there's no `nameID` model:
        if (!m_Meshes.contains(nameID))
        {
            EN_WARN("AssetManager::GetModel() - There's currently no loaded mesh named \"" + nameID + "\"!");
            return Mesh::GetDefaultMesh();
        }

        return m_Meshes.at(nameID).get();
    }
    Texture* en::AssetManager::GetTexture(std::string nameID)
    {
        // If there's no `nameID` texture:
        if (!m_Textures.contains(nameID))
        {
            EN_WARN("AssetManager::GetTexture() - There's currently no loaded texture named \"" + nameID + "\"!");
            return Texture::GetWhiteTexture();
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
}