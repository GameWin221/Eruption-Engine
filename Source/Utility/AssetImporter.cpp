#include "EnPch.hpp"
#include "Utility/AssetImporter.hpp"

namespace en
{
    void AssetImporter::LoadModel(std::string nameID, std::string path, std::string defaultTexture)
    {
        if (m_Models.contains(nameID))
            throw std::runtime_error("Failed to import a model with a name \"" + nameID + "\" because a model with that name already exists!");

        m_Models[nameID] = std::make_unique<Model>(path);

        for (auto& mesh : m_Models[nameID]->m_Meshes)
            mesh.m_Texture = GetTexture(defaultTexture);
    }
    void AssetImporter::LoadTexture(std::string nameID, std::string path)
    {
        if (m_Textures.contains(nameID))
            throw std::runtime_error("Failed to import a texture with a name \"" + nameID + "\" because a texture with that name already exists!");

        m_Textures[nameID] = std::make_unique<Texture>(path);
    }

    void AssetImporter::UnloadModel(std::string nameID)
    {
        m_Models.erase(nameID);
    }
    void AssetImporter::UnloadTexture(std::string nameID)
    {
        m_Textures.erase(nameID);
    }

    Model* AssetImporter::GetModel(std::string nameID)
    {
        return m_Models.at(nameID).get();
    }
    Texture* en::AssetImporter::GetTexture(std::string nameID)
    {
        return m_Textures.at(nameID).get();
    }
}