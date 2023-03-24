#include "Scene.hpp"

namespace en
{
    constexpr float MATRICES_UPDATE_THRESHOLD = 0.25f;
    constexpr float MATERIALS_UPDATE_THRESHOLD = 0.25f;

    constexpr float MATRICES_OVERFLOW_MULTIPLIER = 1.25f;
    constexpr float MATERIALS_OVERFLOW_MULTIPLIER = 1.25f;

    Scene::Scene()
    {
        m_SceneObjects     .reserve(64);
        m_PointLights      .reserve(64);
        m_SpotLights       .reserve(64);
        m_DirectionalLights.reserve(64);
        m_Matrices         .reserve(64);
        m_GPUMaterials     .reserve(64);

        m_GlobalMatricesBuffer = MakeHandle<MemoryBuffer>(
            256U * sizeof(glm::mat4),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
           VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_GlobalMatricesStagingBuffer = MakeHandle<MemoryBuffer>(
            m_GlobalMatricesBuffer->GetSize(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
        m_SingleMatrixStagingBuffer = MakeHandle<MemoryBuffer>(
            sizeof(glm::mat4),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        m_GlobalMaterialsBuffer = MakeHandle<MemoryBuffer>(
            128U * sizeof(GPUMaterial),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_GlobalMaterialsStagingBuffer = MakeHandle<MemoryBuffer>(
            m_GlobalMaterialsBuffer->GetSize(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );
        m_SingleMaterialStagingBuffer = MakeHandle<MemoryBuffer>(
            sizeof(GPUMaterial),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        std::vector<DescriptorInfo::ImageInfoContent> imageViews(MAX_TEXTURES);
        for (auto& view : imageViews)
        {
            view.imageView = AssetManager::Get().GetTexture("SkullAlbedo")->m_Image->GetViewHandle();//AssetManager::Get().GetWhiteSRGBTexture()->m_Image->GetViewHandle();
            view.imageSampler = AssetManager::Get().GetTexture("SkullAlbedo")->m_ImageSampler;//AssetManager::Get().GetWhiteSRGBTexture()->m_ImageSampler;
        }

        // The layout has to be the same as in the GetGlobalDescriptorLayout() method.
        m_GlobalDescriptorSet = MakeHandle<DescriptorSet>(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{
                DescriptorInfo::ImageInfo {
                    .index = 1U,
                    .count = MAX_TEXTURES,

                    .contents = imageViews,

                    .type        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .stage       = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
            },
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index  = 0U,
                    .buffer = m_GlobalMatricesBuffer->GetHandle(),
                    .size   = m_GlobalMatricesBuffer->GetSize(),
                    .type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                },
                DescriptorInfo::BufferInfo {
                    .index  = 2U,
                    .buffer = m_GlobalMaterialsBuffer->GetHandle(),
                    .size   = m_GlobalMaterialsBuffer->GetSize(),
                    .type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            },
        });

        m_MainCamera = MakeHandle<Camera>();
    }
    
    Handle<SceneObject> Scene::CreateSceneObject(const std::string& name, Handle<Mesh> mesh)
    {
        if (m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::CreateSceneObject() - Failed to create a SceneObject because a SceneObject called \"" + name + "\" already exists!");
            return nullptr;
        }

        m_SceneObjects[name] = MakeHandle<SceneObject>(mesh, name);
        
        auto& newSceneObject = m_SceneObjects.at(name);

        newSceneObject->m_MatrixIndex = RegisterMatrix();
        for (auto& subMesh : newSceneObject->m_Mesh->m_SubMeshes)
            subMesh.m_MaterialIndex = RegisterMaterial(subMesh.m_Material);

        EN_LOG("Created a SceneObject called \"" + name + "\"");

        return m_SceneObjects.at(name);
    }
    void Scene::DeleteSceneObject(const std::string& name)
    {
        if (!m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::DeleteSceneObject() - Failed to delete a SceneObject because a SceneObject called \"" + name + "\" doesn't exist!");
            return;
        }

        EN_LOG("Deleted a SceneObject called \"" + name + "\"");

        m_SceneObjects.erase(name);
    }

    Handle<SceneObject> Scene::GetSceneObject(const std::string& name)
    {
        if (!m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::GetSceneObject() - Failed to find a SceneObject named " + name + "!");

            return nullptr;
        }

        return m_SceneObjects.at(name);
    }

    void Scene::RenameSceneObject(const std::string& oldName, const std::string& newName)
    {
        if (m_SceneObjects.contains(newName))
            EN_WARN("Failed to rename " + oldName + " because a SceneObject with a name " + newName + " already exists!")
        else
        {
            if (m_SceneObjects.contains(oldName))
            {
                m_SceneObjects[newName].swap(m_SceneObjects.at(oldName));
                m_SceneObjects.at(newName)->m_Name = newName;
                m_SceneObjects.erase(oldName);
            }
            else
                EN_WARN("Failed to rename " + oldName + " because a SceneObject with that name doesn't exist!")
        }
    }

    PointLight* Scene::CreatePointLight(const glm::vec3 position, const glm::vec3 color, const float intensity, const float radius, const bool active)
    {
        if (m_PointLights.size() + 1 >= MAX_POINT_LIGHTS)
        {
            EN_WARN("Failed to create a new PointLight because you've reached the MAX_POINT_LIGHTS limit (" + std::to_string(MAX_POINT_LIGHTS) + ")!")
            return nullptr;
        }

        m_PointLights.emplace_back(position, color, intensity, radius, active);

        return &m_PointLights.at(m_PointLights.size()-1);
    }
    void Scene::DeletePointLight(const uint32_t index)
    {
        m_PointLights.erase(m_PointLights.begin() + index);
    }

    SpotLight* Scene::CreateSpotLight(const glm::vec3 position, const glm::vec3 direction, const glm::vec3 color, const float innerCutoff, const float outerCutoff, const float range, const float intensity, const bool active)
    {
        if (m_SpotLights.size() + 1 >= MAX_SPOT_LIGHTS)
        {
            EN_WARN("Failed to create a new SpotLight because you've reached the MAX_SPOT_LIGHTS limit (" + std::to_string(MAX_SPOT_LIGHTS) + ")!")
            return nullptr;
        }

        m_SpotLights.emplace_back(position, direction, color, innerCutoff, outerCutoff, range, intensity, active);

        return &m_SpotLights.at(m_SpotLights.size()-1);
    }
    void Scene::DeleteSpotLight(const uint32_t index)
    {
        m_SpotLights.erase(m_SpotLights.begin() + index);
    }

    DirectionalLight* Scene::CreateDirectionalLight(const glm::vec3 direction, const glm::vec3 color, const float intensity, const bool active)
    {
        if (m_DirectionalLights.size() + 1 >= MAX_DIR_LIGHTS)
        {
            EN_WARN("Failed to create a new DirectionalLight because you've reached the MAX_DIR_LIGHTS limit (" + std::to_string(MAX_DIR_LIGHTS) + ")!")
            return nullptr;
        }

        m_DirectionalLights.emplace_back(direction, color, intensity, active);

        return &m_DirectionalLights.back();
    }
    void Scene::DeleteDirectionalLight(const uint32_t index)
    {
        m_DirectionalLights.erase(m_DirectionalLights.begin() + index);
    }

    void Scene::UpdateSceneObjects()
    {
        std::vector<uint32_t> changedMatrixIds{};
        changedMatrixIds.reserve(64);

        for (auto& [name, sceneObject] : m_SceneObjects)
        {
            for (auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
            {
                if (subMesh.m_MaterialChanged)
                {
                    subMesh.m_MaterialIndex = m_RegisteredMaterials.at(subMesh.m_Material->m_Name);
                    subMesh.m_MaterialChanged = false;
                }  
            }

            if (sceneObject->m_TransformChanged)
            {
                glm::mat4 newMatrix = glm::translate(glm::mat4(1.0f), sceneObject->m_Position);
                uint32_t changedMatrixId = sceneObject->m_MatrixIndex;

                if (sceneObject->m_Rotation != glm::vec3(0.0f))
                {
                    newMatrix = glm::rotate(newMatrix, glm::radians(sceneObject->m_Rotation.y), glm::vec3(0, 1, 0));
                    newMatrix = glm::rotate(newMatrix, glm::radians(sceneObject->m_Rotation.z), glm::vec3(0, 0, 1));
                    newMatrix = glm::rotate(newMatrix, glm::radians(sceneObject->m_Rotation.x), glm::vec3(1, 0, 0));
                }

                newMatrix = glm::scale(newMatrix, sceneObject->m_Scale);

                m_Matrices[changedMatrixId] = newMatrix;

                changedMatrixIds.push_back(changedMatrixId);
                sceneObject->m_TransformChanged = false;
            }
        }

        UpdateMatrixBuffer(changedMatrixIds);
    }
    void Scene::UpdateRegisteredAssets()
    {
        std::vector<uint32_t> changedMaterialIds{};
        changedMaterialIds.reserve(64);

        for (uint32_t i = 0U; i < m_Materials.size(); i++)
        {
            auto& cpuMat = m_Materials[i];
            auto& gpuMat = m_GPUMaterials[i];

            if (cpuMat->m_Changed)
            {
                gpuMat.color = cpuMat->m_Color;

                gpuMat.metalnessVal   = cpuMat->GetMetalness();
                gpuMat.roughnessVal   = cpuMat->GetRoughness();
                gpuMat.normalStrength = cpuMat->GetNormalStrength();

                cpuMat->m_AlbedoIndex    = m_RegisteredTextures.at(cpuMat->GetAlbedoTexture()   ->GetName());
                cpuMat->m_RoughnessIndex = m_RegisteredTextures.at(cpuMat->GetRoughnessTexture()->GetName());
                cpuMat->m_MetalnessIndex = m_RegisteredTextures.at(cpuMat->GetMetalnessTexture()->GetName());
                cpuMat->m_NormalIndex    = m_RegisteredTextures.at(cpuMat->GetNormalTexture()   ->GetName());

                gpuMat.albedoId    = cpuMat->GetAlbedoIndex();
                gpuMat.roughnessId = cpuMat->GetRoughnessIndex();
                gpuMat.metalnessId = cpuMat->GetMetalnessIndex();
                gpuMat.normalId    = cpuMat->GetNormalIndex();

                changedMaterialIds.push_back(i);
                cpuMat->m_Changed = false;
            }
        }

        UpdateMaterialBuffer(changedMaterialIds);
    
        if (m_TexturesChanged)
        {
            UpdateGlobalDescriptor();
            m_TexturesChanged = false;
        }
    }
    VkDescriptorSetLayout Scene::GetGlobalDescriptorLayout()
    {
        std::vector<DescriptorInfo::ImageInfoContent> imageViews(MAX_TEXTURES);

        return DescriptorAllocator::Get().MakeLayout(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{
                DescriptorInfo::ImageInfo {
                    .index = 1U,
                    .count = MAX_TEXTURES,

                    .contents = imageViews,

                    .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
            },
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                },
                DescriptorInfo::BufferInfo {
                    .index = 2U,
                    .type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            },
        });
    }

    uint32_t Scene::RegisterMatrix(const glm::mat4& matrix)
    {
        m_Matrices.emplace_back(matrix);
        return static_cast<uint32_t>(m_Matrices.size()) - 1U;
    }
    uint32_t Scene::RegisterMaterial(Handle<Material> material)
    {
        if (!m_RegisteredMaterials.contains(material->GetName()))
        {
            material->m_AlbedoIndex    = RegisterTexture(material->GetAlbedoTexture());
            material->m_RoughnessIndex = RegisterTexture(material->GetRoughnessTexture());
            material->m_MetalnessIndex = RegisterTexture(material->GetMetalnessTexture());
            material->m_NormalIndex    = RegisterTexture(material->GetNormalTexture());

            m_GPUMaterials.emplace_back(GPUMaterial{});
            m_Materials.emplace_back(material);

            m_RegisteredMaterials[material->GetName()] = static_cast<uint32_t>(m_Materials.size()) - 1U;
        }

        return m_RegisteredMaterials.at(material->GetName());
    }
    uint32_t Scene::RegisterTexture(Handle<Texture> texture)
    {
        if (!m_RegisteredTextures.contains(texture->GetName()))
        {
            m_Textures.emplace_back(texture);
            m_RegisteredTextures[texture->GetName()] = static_cast<uint32_t>(m_Textures.size()) - 1U;
            m_TexturesChanged = true;
        }

        return m_RegisteredTextures.at(texture->GetName());
    }

    void Scene::DeregisterMatrix(uint32_t index)
    {

    }
    void Scene::DeregisterMaterial(uint32_t index)
    {

    }
    void Scene::DeregisterTexture(uint32_t index)
    {

    }

    void Scene::UpdateMatrixBuffer(const std::vector<uint32_t>& changedMatrixIds)
    {
        if (m_Matrices.size() == 0)
            return;

        if (sizeof(glm::mat4) * m_Matrices.size() > m_GlobalMatricesBuffer->GetSize())
        {
            EN_LOG("MATRIX RESIZE");

            uint32_t overflow = sizeof(glm::mat4) * m_Matrices.size() - m_GlobalMatricesBuffer->GetSize();

            m_GlobalMatricesBuffer->Resize((m_GlobalMatricesBuffer->GetSize() + overflow)*MATRICES_OVERFLOW_MULTIPLIER);
            UpdateGlobalDescriptor();
        }

        uint32_t totalMatrices   = m_Matrices.size();
        uint32_t changedMatrices = changedMatrixIds.size();

        // If more than MATRICES_UPDATE_THRESHOLD percent of matrices have changed, do a whole copy
        if ((float)changedMatrices / totalMatrices > MATRICES_UPDATE_THRESHOLD)
        {
            EN_LOG("TOTAL MATRIX UPDATE");
            m_GlobalMatricesStagingBuffer->MapMemory(m_Matrices.data(), sizeof(glm::mat4) * m_Matrices.size());
            m_GlobalMatricesStagingBuffer->CopyTo(m_GlobalMatricesBuffer, sizeof(glm::mat4) * m_Matrices.size());
        }
        else 
        {
            for (const auto& changedMatrixId : changedMatrixIds)
            {
                EN_LOG("SINGLE MATRIX UPDATE");
                m_SingleMatrixStagingBuffer->MapMemory(&m_Matrices[changedMatrixId], sizeof(glm::mat4));
                m_SingleMatrixStagingBuffer->CopyTo(m_GlobalMatricesBuffer, sizeof(glm::mat4), 0U, changedMatrixId * sizeof(glm::mat4));
            }
        }
    }
    void Scene::UpdateMaterialBuffer(const std::vector<uint32_t>& changedMaterialIds)
    {
        if (m_Materials.size() == 0)
            return;
    
        if (sizeof(GPUMaterial) * m_Materials.size() > m_GlobalMaterialsBuffer->GetSize())
        {
            EN_LOG("MATERIAL RESIZE");

            uint32_t overflow = sizeof(GPUMaterial) * m_Materials.size() - m_GlobalMaterialsBuffer->GetSize();

            m_GlobalMaterialsBuffer->Resize((m_GlobalMaterialsBuffer->GetSize() + overflow) * MATERIALS_OVERFLOW_MULTIPLIER);
            UpdateGlobalDescriptor();
        }

        uint32_t totalMaterials   = m_Materials.size();
        uint32_t changedMaterials = changedMaterialIds.size();

        // If more than MATRICES_UPDATE_THRESHOLD percent of matrices have changed, do a whole copy
        if ((float)changedMaterials / totalMaterials > MATERIALS_UPDATE_THRESHOLD)
        {
            EN_LOG("TOTAL MATERIAL UPDATE");
            m_GlobalMaterialsStagingBuffer->MapMemory(m_GPUMaterials.data(), sizeof(GPUMaterial) * m_Materials.size());
            m_GlobalMaterialsStagingBuffer->CopyTo(m_GlobalMaterialsBuffer, sizeof(GPUMaterial) * m_Materials.size());
        }
        else
        {
            for (const auto& changedMaterialId : changedMaterialIds)
            {
                EN_LOG("SINGLE MATERIAL UPDATE");
                m_SingleMaterialStagingBuffer->MapMemory(&m_GPUMaterials[changedMaterialId], sizeof(GPUMaterial));
                m_SingleMaterialStagingBuffer->CopyTo(m_GlobalMaterialsBuffer, sizeof(GPUMaterial), 0U, changedMaterialId * sizeof(GPUMaterial));
            }
        }
    }
    void Scene::UpdateGlobalDescriptor()
    {
        std::vector<DescriptorInfo::ImageInfoContent> imageViews(MAX_TEXTURES);

        uint32_t i = 0U;
        for (; i < m_Textures.size(); i++)
        {
            imageViews[i].imageView    = m_Textures[i]->m_Image->GetViewHandle();
            imageViews[i].imageSampler = m_Textures[i]->m_ImageSampler;
        }
        for (; i < MAX_TEXTURES; i++)
        {
            imageViews[i].imageView    = AssetManager::Get().GetWhiteNonSRGBTexture()->m_Image->GetViewHandle();
            imageViews[i].imageSampler = AssetManager::Get().GetWhiteNonSRGBTexture()->m_ImageSampler;
        }

        m_GlobalDescriptorSet->Update(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{
                DescriptorInfo::ImageInfo {
                    .index = 1U,
                    .count = MAX_TEXTURES,

                    .contents = imageViews,

                    .type        = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                    .stage       = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                },
            },
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index  = 0U,
                    .buffer = m_GlobalMatricesBuffer->GetHandle(),
                    .size   = m_GlobalMatricesBuffer->GetSize(),
                    .type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage  = VK_SHADER_STAGE_VERTEX_BIT,
                },
                DescriptorInfo::BufferInfo {
                    .index  = 2U,
                    .buffer = m_GlobalMaterialsBuffer->GetHandle(),
                    .size   = m_GlobalMaterialsBuffer->GetSize(),
                    .type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            },
        });
    }
}