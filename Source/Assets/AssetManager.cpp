#include <Core/EnPch.hpp>
#include "AssetManager.hpp"

namespace en
{
    void AssetManager::LoadModel(std::string nameID, std::string path, std::string defaultTexture)
    {
        if (m_Models.contains(nameID))
        {
            std::cout << "AssetManager::LoadModel() - Failed to load a model with a name \"" << nameID << "\" because a model with that name already exists!\n";
            return;
        }

        m_Models[nameID] = std::make_unique<Model>(path);

        for (auto& mesh : m_Models[nameID]->m_Meshes)
            mesh.m_Texture = (defaultTexture != "WHITE_TEXTURE") ? GetTexture(defaultTexture) : Texture::GetWhiteTexture();
    }
    void AssetManager::LoadTexture(std::string nameID, std::string path, bool loadFlipped)
    {
        if (m_Textures.contains(nameID))
        {
            std::cout << "AssetManager::LoadTexture() - Failed to load a texture with a name \"" << nameID << "\" because a texture with that name already exists!\n";
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
}