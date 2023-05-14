#include "AssetManager.hpp"

namespace en
{
    constexpr uint64_t WHITE_PIXEL = 0xffffffffffffffff; // UINT64_MAX;
   
    AssetManager* g_AssetManagerInstance = nullptr;

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
    AssetManager& AssetManager::Get()
    {
        return *g_AssetManagerInstance;
    }

    bool AssetManager::LoadMesh(const std::string& nameID, const std::string& path, const MeshImportProperties& properties)
    {
        if (m_Meshes.contains(nameID))
        {
            EN_WARN("AssetManager::LoadMesh() - Failed to load a mesh with name \"" + nameID + "\" from \"" + path + "\" because a model with that name already exists!");
            return false;
        }

        GLTFImporter importer(properties, GetDefaultMaterial(), GetWhiteSRGBTexture(), GetWhiteNonSRGBTexture());
        MeshData data = importer.LoadMeshFromFile(path, nameID);

        m_Meshes[nameID] = data.mesh;

        for (const auto& texture : data.textures)
        {
            if (m_Textures.contains(texture->GetName()))
            {
                EN_WARN("AssetManager::LoadMesh() - Failed to load a texture with name \"" + texture->GetName() + "\" from \"" + texture->GetFilePath() + "\" because a texture with that name already exists!");
                return false;
            }

            m_Textures[texture->GetName()] = texture;
        }
        for (const auto& material : data.materials)
        {
            if (m_Materials.contains(material->GetName()))
            {
                EN_WARN("AssetManager::LoadMesh() - Failed to create a material with name \"" + material->GetName() + "\" because a material with that name already exists!");
                return false;
            }

            m_Materials[material->GetName()] = material;
        }

        return true;
    }
    bool AssetManager::LoadTexture(const std::string& nameID, const std::string& path, const TextureImportProperties& properties)
    {
        if (m_Textures.contains(nameID))
        {
            EN_WARN("AssetManager::LoadTexture() - Failed to load a texture with name \"" + nameID + "\" from \"" + path + "\" because a texture with that name already exists!");
            return false;
        }

        m_Textures[nameID] = MakeHandle<Texture>(path, nameID, static_cast<VkFormat>(properties.format), properties.flipped);
        return true;
    }

    void AssetManager::DeleteMesh(const std::string& nameID)
    {
        EN_WARN("AssetManager::DeleteMesh() - This feature is disabled for now!");
    }
    void AssetManager::DeleteTexture(const std::string& nameID)
    {
        EN_WARN("AssetManager::DeleteTexture() - This feature is disabled for now!");
    }

    bool AssetManager::CreateMaterial(const std::string& nameID, const glm::vec3 color, const float metalnessVal, const float roughnessVal, const float normalStrength, Handle<Texture> albedoTexture, Handle<Texture> roughnessTexture, Handle<Texture> normalTexture, Handle<Texture> metalnessTexture)
    {
        if (!albedoTexture)
            albedoTexture = GetWhiteSRGBTexture();
        if (!roughnessTexture)
            roughnessTexture = GetWhiteNonSRGBTexture();
        if(!normalTexture)
            normalTexture = GetWhiteNonSRGBTexture();
        if(!metalnessTexture)
            metalnessTexture = GetWhiteNonSRGBTexture();
       
        if (m_Materials.contains(nameID))
        {
            EN_WARN("AssetManager::CreateMaterial() - Failed to create a material with name \"" + nameID + "\" because a material with that name already exists!");
            return false;
        }

        m_Materials[nameID] = MakeHandle<Material>(nameID, color, metalnessVal, roughnessVal, normalStrength, albedoTexture, roughnessTexture, normalTexture, metalnessTexture);

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
    void AssetManager::DeleteMaterial(const std::string& nameID)
    {
        EN_WARN("AssetManager::DeleteMaterial() - This feature is disabled for now!");
    }
    /*
    void AssetManager::RenameMesh(const std::string& oldNameID, const std::string& newNameID)
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
    void AssetManager::RenameTexture(const std::string& oldNameID, const std::string& newNameID)
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
    void AssetManager::RenameMaterial(const std::string& oldNameID, const std::string& newNameID)
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
    */
    Handle<Mesh> AssetManager::GetMesh(const std::string& nameID)
    {
        // If there's no `nameID` model:
        if (!m_Meshes.contains(nameID))
        {
            EN_WARN("AssetManager::GetModel() - There's currently no loaded mesh named \"" + nameID + "\"!");

            return Mesh::GetEmptyMesh();
        }

        return m_Meshes.at(nameID);
    }
    Handle<Texture> en::AssetManager::GetTexture(const std::string& nameID)
    {
        // If there's no `nameID` texture:
        if (!m_Textures.contains(nameID))
        {
            EN_WARN("AssetManager::GetTexture() - There's currently no loaded texture named \"" + nameID + "\"!");
            return GetWhiteNonSRGBTexture();
        }

        return m_Textures.at(nameID);
    }
    Handle<Material> AssetManager::GetMaterial(const std::string& nameID)
    {
        // If there's no `nameID` material:
        if (!m_Materials.contains(nameID))
        {
            EN_WARN("AssetManager::GetMaterial() - There's currently no material named \"" + nameID + "\"!");
            return GetDefaultMaterial();
        }

        return m_Materials.at(nameID);
    }

    std::vector<Handle<Mesh>> AssetManager::GetAllMeshes()
    {
        std::vector<Handle<Mesh>> meshes(m_Meshes.size());

        for (int i = 0; const auto& [name, mesh] : m_Meshes)
            meshes[i++] = mesh;
        
        return meshes;
    }
    std::vector<Handle<Texture>> AssetManager::GetAllTextures()
    {
        std::vector<Handle<Texture>> textures(m_Textures.size());

        for (int i = 0; const auto& [name, texture] : m_Textures)
            textures[i++] = texture;

        return textures;
    }
    std::vector<Handle<Material>> AssetManager::GetAllMaterials()
    {
        std::vector<Handle<Material>> materials(m_Materials.size());

        for (int i = 0; const auto & [name, material] : m_Materials)
            materials[i++] = material;

        return materials;
    }

    Handle<Texture> AssetManager::GetWhiteSRGBTexture()
    {
        if (!m_SRGBWhiteTexture)
            m_SRGBWhiteTexture = MakeHandle<Texture>((uint8_t*)&WHITE_PIXEL, "_DefaultWhiteSRGB", VK_FORMAT_R8G8B8A8_SRGB, VkExtent2D{ 1U, 1U }, false);

        return m_SRGBWhiteTexture;
    }
    Handle<Texture> AssetManager::GetWhiteNonSRGBTexture()
    {
        if (!m_NSRGBTexture)
            m_NSRGBTexture = MakeHandle<Texture>((uint8_t*)&WHITE_PIXEL, "_DefaultWhiteNonSRGB", VK_FORMAT_R8G8B8A8_UNORM, VkExtent2D{ 1U, 1U }, false);

        return m_NSRGBTexture;
    }

    Handle<Material> AssetManager::GetDefaultMaterial()
    {
        if (!m_DefaultMaterial)
            m_DefaultMaterial = MakeHandle<Material>("No Material", glm::vec3(1.0f), 0.0f, 0.75f, 0.0f, GetWhiteSRGBTexture(), GetWhiteNonSRGBTexture(), GetWhiteNonSRGBTexture(), GetWhiteNonSRGBTexture());

        return m_DefaultMaterial;
    }
}