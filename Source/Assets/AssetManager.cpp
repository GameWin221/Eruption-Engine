#include <Core/EnPch.hpp>
#include "AssetManager.hpp"

namespace en
{
    void AssetManager::LoadModel(std::string nameID, std::string path)
    {
        if (m_Models.contains(nameID))
        {
            std::cout << "AssetManager::LoadModel() - Failed to load a model with name \"" << nameID << "\" because a model with that name already exists!\n";
            return;
        }

        m_Models[nameID] = std::make_unique<Model>(path);
    }
    void AssetManager::LoadTexture(std::string nameID, std::string path, bool loadFlipped)
    {
        if (m_Textures.contains(nameID))
        {
            std::cout << "AssetManager::LoadTexture() - Failed to load a texture with name \"" << nameID << "\" because a texture with that name already exists!\n";
            return;
        }

        m_Textures[nameID] = std::make_unique<Texture>(path, loadFlipped);
    }

    void AssetManager::UnloadModel(std::string nameID)
    {
        m_Models.erase(nameID);
    }
    void AssetManager::UnloadTexture(std::string nameID)
    {
        m_Textures.erase(nameID);
    }

    void AssetManager::CreateMaterial(std::string nameID, glm::vec3 color, Texture* albedoTexture, Texture* specularTexture)
    {
        if (m_Materials.contains(nameID))
        {
            std::cout << "AssetManager::CreateMaterial() - Failed to create a material with name \"" << nameID << "\" because a material with that name already exists!\n";
            return;
        }

        m_Materials[nameID] = std::make_unique<Material>(color, albedoTexture, specularTexture);
    }
    void AssetManager::DeleteMaterial(std::string nameID)
    {
        m_Materials.erase(nameID);
    }

    Model* AssetManager::GetModel(std::string nameID)
    {
        // If there's no `nameID` model:
        if (!m_Models.contains(nameID))
        {
            std::cout << "AssetManager::GetModel() - There's currently no loaded model named \"" << nameID << "\"!\n";
            return Model::GetNoModel();
        }

        return m_Models.at(nameID).get();
    }
    Texture* en::AssetManager::GetTexture(std::string nameID)
    {
        // If there's no `nameID` texture:
        if (!m_Textures.contains(nameID))
        {
            std::cout << "AssetManager::GetTexture() - There's currently no loaded texture named \"" << nameID << "\"!\n";
            return Texture::GetWhiteTexture();
        }

        return m_Textures.at(nameID).get();
    }
    Material* AssetManager::GetMaterial(std::string nameID)
    {
        // If there's no `nameID` material:
        if (!m_Materials.contains(nameID))
        {
            std::cout << "AssetManager::GetMaterial() - There's currently no material named \"" << nameID << "\"!\n";
            return Material::GetDefaultMaterial();
        }

        return m_Materials.at(nameID).get();
    }
}