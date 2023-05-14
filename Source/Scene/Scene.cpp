#include "Scene.hpp"

namespace en
{
    constexpr float MATRICES_UPDATE_THRESHOLD  = 0.25f;
    constexpr float MATERIALS_UPDATE_THRESHOLD = 0.25f;

    constexpr float POINT_LIGHTS_UPDATE_THRESHOLD = 0.25f;
    constexpr float SPOT_LIGHTS_UPDATE_THRESHOLD  = 0.25f;
    constexpr float DIR_LIGHTS_UPDATE_THRESHOLD   = 0.25f;

    constexpr float MATRICES_OVERFLOW_MULTIPLIER = 1.2f;
    constexpr float MATERIALS_OVERFLOW_MULTIPLIER = 1.2f;

    Scene::Scene()
    {
        m_SceneObjects     .reserve(64);
        m_PointLights      .reserve(64);
        m_SpotLights       .reserve(64);
        m_DirectionalLights.reserve(64);

        m_Matrices    .resize(64);
        m_Textures    .resize(64);
        m_Materials   .resize(64);
        m_GPUMaterials.resize(64);

        m_GlobalMatricesBuffer = MakeHandle<MemoryBuffer>(
            256U * sizeof(glm::mat4),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
           VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_GlobalMatricesStagingBuffer = MakeHandle<MemoryBuffer>(
            m_GlobalMatricesBuffer->GetSize(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        m_GlobalMaterialsBuffer = MakeHandle<MemoryBuffer>(
            256U * sizeof(GPUMaterial),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_GlobalMaterialsStagingBuffer = MakeHandle<MemoryBuffer>(
            m_GlobalMaterialsBuffer->GetSize(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU
        );

        m_LightsBuffer = MakeHandle<MemoryBuffer>(
            sizeof(GPULights),
            VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
            VMA_MEMORY_USAGE_GPU_ONLY
        );
        m_LightsStagingBuffer = MakeHandle<MemoryBuffer>(
            m_LightsBuffer->GetSize(),
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU  
        );

        m_LightingDescriptorSet = MakeHandle<DescriptorSet>(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{},
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .buffer = m_GlobalMatricesBuffer->GetHandle(),
                    .size = m_GlobalMatricesBuffer->GetSize(),
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                },
                DescriptorInfo::BufferInfo {
                    .index = 1U,
                    .buffer = m_LightsBuffer->GetHandle(),
                    .size = m_LightsBuffer->GetSize(),
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                }
            },
        });

        std::vector<DescriptorInfo::ImageInfoContent> imageViews(MAX_TEXTURES);
        for (auto& view : imageViews)
        {
            view.imageView = AssetManager::Get().GetTexture("SkullAlbedo")->m_Image->GetViewHandle();//AssetManager::Get().GetWhiteSRGBTexture()->m_Image->GetViewHandle();
            view.imageSampler = AssetManager::Get().GetTexture("SkullAlbedo")->m_Sampler->GetHandle();//AssetManager::Get().GetWhiteSRGBTexture()->m_ImageSampler;
        }

        m_LightsBufferDescriptorSet = MakeHandle<DescriptorSet>(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{},
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .buffer = m_LightsBuffer->GetHandle(),
                    .size = m_LightsBuffer->GetSize(),
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                },
            }
        });

        // The layout has to be the same as in the GetGlobalDescriptorLayout() method.
        m_GlobalDescriptorSet = MakeHandle<DescriptorSet>(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{
                DescriptorInfo::ImageInfo {
                    .index = 1U,
                    .count = static_cast<uint32_t>(imageViews.size()),

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
                DescriptorInfo::BufferInfo {
                    .index  = 3U,
                    .buffer = m_LightsBuffer->GetHandle(),
                    .size   = m_LightsBuffer->GetSize(),
                    .type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            },
        });
        
        m_MainCamera = MakeHandle<Camera>();
    }
    
    Handle<SceneObject> Scene::CreateSceneObject(const std::string& name, Handle<Mesh> mesh)
    {
        std::string finalName = name;

        if (m_SceneObjects.contains(finalName))
        {
            EN_WARN("Scene::CreateSceneObject() - Failed to create a SceneObject because a SceneObject called \"" + name + "\" already exists!");
            return nullptr;
        }

        m_SceneObjects[finalName] = MakeHandle<SceneObject>(mesh, finalName);
        
        auto& newSceneObject = m_SceneObjects.at(finalName);

        newSceneObject->m_MatrixIndex = RegisterMatrix();
        for (auto& subMesh : newSceneObject->m_Mesh->m_SubMeshes)
            subMesh.m_MaterialIndex = RegisterMaterial(subMesh.m_Material);

        EN_LOG("Created a SceneObject called \"" + finalName + "\"");

        return m_SceneObjects.at(finalName);
    }
    void Scene::DeleteSceneObject(const std::string& name)
    {
        if (!m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::DeleteSceneObject() - Failed to delete a SceneObject because a SceneObject called \"" + name + "\" doesn't exist!");
            return;
        }

        EN_LOG("Deleted a SceneObject called \"" + name + "\"");

        DeregisterMatrix(m_SceneObjects.at(name)->GetMatrixIndex());
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
    /*
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
    */
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

    void Scene::UpdateSceneCPU()
    {
        m_ActivePointLightsShadowIDs.clear();
        m_ActiveSpotLightsShadowIDs.clear();
        m_ActiveDirLightsShadowIDs.clear();

        m_ChangedMatrixIDs.clear();
        m_ChangedMaterialIDs.clear();

        for (auto& [name, sceneObject] : m_SceneObjects)
        {
            for (auto& subMesh : sceneObject->m_Mesh->m_SubMeshes)
            {
                if (subMesh.m_MaterialChanged)
                {
                    if (m_RegisteredMaterials.contains(subMesh.m_Material->m_Name))
                        subMesh.m_MaterialIndex = m_RegisteredMaterials.at(subMesh.m_Material->m_Name);
                    else
                        subMesh.m_MaterialIndex = RegisterMaterial(subMesh.m_Material);

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

                m_ChangedMatrixIDs.push_back(changedMatrixId);
                sceneObject->m_TransformChanged = false;
            }
        }

        for (const auto& i : m_OccupiedMaterials)
        {
            auto& cpuMat = m_Materials[i];
            auto& gpuMat = m_GPUMaterials[i];

            if (cpuMat->m_Changed)
            {
                gpuMat.color = cpuMat->m_Color;

                const std::string& albedoName = cpuMat->GetAlbedoTexture()->GetName();
                const std::string& roughnessName = cpuMat->GetRoughnessTexture()->GetName();
                const std::string& metalnessName = cpuMat->GetMetalnessTexture()->GetName();
                const std::string& normalName = cpuMat->GetNormalTexture()->GetName();
                const std::string& defaultSRGBName = AssetManager::Get().GetWhiteSRGBTexture()->GetName();
                const std::string& defaultNonSRGBName = AssetManager::Get().GetWhiteNonSRGBTexture()->GetName();

                gpuMat.metalnessVal = metalnessName == defaultNonSRGBName ? cpuMat->GetMetalness() : 1.0f;
                gpuMat.roughnessVal = roughnessName == defaultNonSRGBName ? cpuMat->GetRoughness() : 1.0f;
                gpuMat.normalStrength = normalName != defaultNonSRGBName ? cpuMat->GetNormalStrength() : 0.0f;

                if (!m_RegisteredTextures.contains(albedoName))
                    RegisterTexture(cpuMat->GetAlbedoTexture());
                if (!m_RegisteredTextures.contains(roughnessName))
                    RegisterTexture(cpuMat->GetRoughnessTexture());
                if (!m_RegisteredTextures.contains(metalnessName))
                    RegisterTexture(cpuMat->GetMetalnessTexture());
                if (!m_RegisteredTextures.contains(normalName))
                    RegisterTexture(cpuMat->GetNormalTexture());

                cpuMat->m_AlbedoIndex = m_RegisteredTextures.at(albedoName);
                cpuMat->m_RoughnessIndex = m_RegisteredTextures.at(roughnessName);
                cpuMat->m_MetalnessIndex = m_RegisteredTextures.at(metalnessName);
                cpuMat->m_NormalIndex = m_RegisteredTextures.at(normalName);

                gpuMat.albedoId = cpuMat->GetAlbedoIndex();
                gpuMat.roughnessId = cpuMat->GetRoughnessIndex();
                gpuMat.metalnessId = cpuMat->GetMetalnessIndex();
                gpuMat.normalId = cpuMat->GetNormalIndex();

                m_ChangedMaterialIDs.push_back(i);
                cpuMat->m_Changed = false;
            }
        }

        m_SceneLightingChanged = false;
        m_ChangedPointLightsIDs.clear();
        m_ChangedSpotLightsIDs .clear();
        m_ChangedDirLightsIDs  .clear();

        uint32_t oldActivePointLights = m_GPULights.activePointLights;
        uint32_t oldActiveSpotLights  = m_GPULights.activeSpotLights;
        uint32_t oldActiveDirLights   = m_GPULights.activeDirLights;

        m_GPULights.activePointLights = 0U;
        m_GPULights.activeSpotLights  = 0U;
        m_GPULights.activeDirLights   = 0U;

        if (m_GPULights.ambientLight != m_AmbientColor)
        {
            m_GPULights.ambientLight = m_AmbientColor;
            m_SceneLightingChanged = true;
        }

        for (auto& l : m_PointLights)
            l.m_ShadowmapIndex = -1;
        for (auto& l : m_SpotLights)
            l.m_ShadowmapIndex = -1;
        for (auto& l : m_DirectionalLights)
            l.m_ShadowmapIndex = -1;

        uint32_t pointShadowCasters = 0U;
        uint32_t spotShadowCasters = 0U;
        uint32_t dirShadowCasters = 0U;

        for (uint32_t i = 0U; auto& light : m_PointLights)
        {
            PointLight::Buffer& buffer = m_GPULights.pointLights[m_GPULights.activePointLights];

            glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;
            float     lightRad = light.m_Radius * (float)light.m_Active;

            i++;

            if (lightCol == glm::vec3(0.0) || lightRad <= 0.0f)
                continue;

            if (pointShadowCasters < MAX_POINT_LIGHT_SHADOWS && light.m_CastShadows)
            {
                light.m_ShadowmapIndex = pointShadowCasters++;
                m_ActivePointLightsShadowIDs.push_back(i - 1);
            }


            if (buffer.position != light.m_Position ||
                buffer.color != lightCol ||
                buffer.radius != lightRad ||
                buffer.shadowmapIndex != light.m_ShadowmapIndex ||
                buffer.shadowSoftness != light.m_ShadowSoftness ||
                buffer.pcfSampleRate != light.m_PCFSampleRate ||
                buffer.bias != light.m_ShadowBias
            ) {
                m_ChangedPointLightsIDs.push_back(m_GPULights.activePointLights);

                buffer.position = light.m_Position;
                buffer.color = lightCol;
                buffer.radius = lightRad;
                buffer.shadowmapIndex = light.m_ShadowmapIndex;
                buffer.shadowSoftness = light.m_ShadowSoftness;
                buffer.pcfSampleRate = light.m_PCFSampleRate;
                buffer.bias = light.m_ShadowBias;

                if (light.m_ShadowmapIndex != -1)
                {
                    const glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light.m_Radius);

                    buffer.viewProj[0] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 1.0,  0.0,  0.0), glm::vec3(0.0, -1.0,  0.0));
                    buffer.viewProj[1] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3(-1.0,  0.0,  0.0), glm::vec3(0.0, -1.0,  0.0));
                    buffer.viewProj[2] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0,  1.0,  0.0), glm::vec3(0.0,  0.0,  1.0));
                    buffer.viewProj[3] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0, -1.0,  0.0), glm::vec3(0.0,  0.0, -1.0));
                    buffer.viewProj[4] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0,  0.0,  1.0), glm::vec3(0.0, -1.0,  0.0));
                    buffer.viewProj[5] = proj * glm::lookAt(light.m_Position, light.m_Position + glm::vec3( 0.0,  0.0, -1.0), glm::vec3(0.0, -1.0,  0.0)); 
                }
            }

            m_GPULights.activePointLights++;
        }
        for (uint32_t i = 0U; auto& light : m_SpotLights)
        {
            SpotLight::Buffer& buffer = m_GPULights.spotLights[m_GPULights.activeSpotLights];

            glm::vec3 lightColor = light.m_Color * (float)light.m_Active * light.m_Intensity;

            i++;

            if (light.m_Range == 0.0f || lightColor == glm::vec3(0.0) || light.m_OuterCutoff == 0.0f)
                continue;

            if (spotShadowCasters < MAX_SPOT_LIGHT_SHADOWS && light.m_CastShadows)
            {
                light.m_ShadowmapIndex = spotShadowCasters++;
                m_ActiveSpotLightsShadowIDs.push_back(i - 1);
            }

            if (buffer.color != lightColor ||
                buffer.range != light.m_Range ||
                buffer.outerCutoff != light.m_OuterCutoff ||
                buffer.position != light.m_Position ||
                buffer.innerCutoff != light.m_InnerCutoff ||
                buffer.direction != -glm::normalize(light.m_Direction) ||
                buffer.shadowmapIndex != light.m_ShadowmapIndex ||
                buffer.shadowSoftness != light.m_ShadowSoftness ||
                buffer.pcfSampleRate != light.m_PCFSampleRate ||
                buffer.bias != light.m_ShadowBias
            ) {
                m_ChangedSpotLightsIDs.push_back(m_GPULights.activeSpotLights);

                buffer.color          = lightColor;
                buffer.range          = light.m_Range;
                buffer.outerCutoff    = light.m_OuterCutoff;
                buffer.position       = light.m_Position;
                buffer.innerCutoff    = light.m_InnerCutoff;
                buffer.direction      = -glm::normalize(light.m_Direction);
                buffer.shadowmapIndex = light.m_ShadowmapIndex;
                buffer.shadowSoftness = light.m_ShadowSoftness;
                buffer.pcfSampleRate  = light.m_PCFSampleRate;
                buffer.bias           = light.m_ShadowBias;

                if (light.m_ShadowmapIndex != -1)
                {
                    glm::mat4 view = glm::lookAt(light.m_Position, light.m_Position + light.m_Direction + glm::vec3(0.00000127f), glm::vec3(0.0, 1.0, 0.0));
                    glm::mat4 proj = glm::perspective((light.m_OuterCutoff + 0.02f) * glm::pi<float>(), 1.0f, 0.01f, light.m_Range);
                    buffer.lightMat = proj * view;
                }
            }

            m_GPULights.activeSpotLights++;
        }
        for (uint32_t i = 0U; auto& light : m_DirectionalLights)
        {
            DirectionalLight::Buffer& buffer = m_GPULights.dirLights[m_GPULights.activeDirLights];

            glm::vec3 lightCol = light.m_Color * (float)light.m_Active * light.m_Intensity;

            i++;

            if (lightCol == glm::vec3(0.0f))
                continue;

            if (dirShadowCasters < MAX_DIR_LIGHT_SHADOWS && light.m_CastShadows)
            {
                light.m_ShadowmapIndex = dirShadowCasters++;
                m_ActiveDirLightsShadowIDs.push_back(i - 1);
            }

            if (buffer.color != lightCol ||
                buffer.shadowmapIndex != light.m_ShadowmapIndex ||
                buffer.direction != glm::normalize(light.m_Direction) ||
                buffer.shadowSoftness != light.m_ShadowSoftness ||
                buffer.pcfSampleRate != light.m_PCFSampleRate ||
                buffer.bias != light.m_ShadowBias
            ) {
                m_ChangedDirLightsIDs.push_back(m_GPULights.activeDirLights);

                buffer.color          = lightCol;
                buffer.shadowmapIndex = light.m_ShadowmapIndex;
                buffer.direction      = glm::normalize(light.m_Direction);
                buffer.shadowSoftness = light.m_ShadowSoftness;
                buffer.pcfSampleRate  = light.m_PCFSampleRate;
                buffer.bias           = light.m_ShadowBias;
            }
                
            m_GPULights.activeDirLights++;
        }

        if (oldActivePointLights != m_GPULights.activePointLights ||
            oldActiveSpotLights != m_GPULights.activeSpotLights ||
            oldActiveDirLights != m_GPULights.activeDirLights
        ) {
            if(m_GPULights.activePointLights < oldActivePointLights)
                m_ChangedPointLightsIDs.push_back(m_GPULights.activePointLights);

            if (m_GPULights.activeSpotLights < oldActiveSpotLights)
                m_ChangedSpotLightsIDs.push_back(m_GPULights.activeSpotLights);

            m_SceneLightingChanged = true;
        }

        // Reset last (point/spot)light for clustered rendering
        m_GPULights.pointLights[m_GPULights.activePointLights].color  = glm::vec3(0.0f);
        m_GPULights.pointLights[m_GPULights.activePointLights].radius = 0.0f;

        m_GPULights.spotLights[m_GPULights.activeSpotLights].color      = glm::vec3(0.0f);
        m_GPULights.spotLights[m_GPULights.activeSpotLights].range      = 0.0f;
        m_GPULights.spotLights[m_GPULights.activeSpotLights].direction  = glm::vec3(0.0f);
        m_GPULights.spotLights[m_GPULights.activeSpotLights].outerCutoff = 0.0f; 
    }
    void Scene::UpdateSceneGPU()
    {
        UpdateMatrixBuffer(m_ChangedMatrixIDs);

        UpdateLightsBuffer(m_ChangedPointLightsIDs, m_ChangedSpotLightsIDs, m_ChangedDirLightsIDs);

        UpdateMaterialBuffer(m_ChangedMaterialIDs);

        if (m_GlobalDescriptorChanged)
        {
            UpdateGlobalDescriptor();
            m_GlobalDescriptorChanged = false;
        }
    }

    VkDescriptorSetLayout Scene::GetGlobalDescriptorLayout()
    {
        std::vector<DescriptorInfo::ImageInfoContent> imageViews(MAX_TEXTURES);

        return DescriptorAllocator::Get().MakeLayout(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{
                DescriptorInfo::ImageInfo {
                    .index = 1U,
                    .count = static_cast<uint32_t>(imageViews.size()),

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
                DescriptorInfo::BufferInfo {
                    .index = 3U,
                    .type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            },
            //0,
            //VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT
        });
    }
    VkDescriptorSetLayout Scene::GetLightingDescriptorLayout()
    {
        return DescriptorAllocator::Get().MakeLayout(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{},
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                },
                DescriptorInfo::BufferInfo {
                    .index = 1U,
                    .type  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                }
            },
        });
    }
    VkDescriptorSetLayout Scene::GetLightsBufferDescriptorLayout()
    {
        return DescriptorAllocator::Get().MakeLayout(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{},
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                },
            }
        });
    }

    uint32_t Scene::RegisterMatrix(const glm::mat4& matrix)
    {
        uint32_t index = 0U;
        while (m_OccupiedMatrices.contains(index))
            index++;

        if (index > m_Matrices.size() - 1)
            m_Matrices.resize(m_Matrices.size() * 2);

        m_OccupiedMatrices.insert(index);
        m_Matrices[index] = matrix;
        
        return index;
    }
    uint32_t Scene::RegisterMaterial(Handle<Material> material)
    {
        if (!m_RegisteredMaterials.contains(material->GetName()))
        {
            uint32_t index = 0U;
            while (m_OccupiedMaterials.contains(index))
                index++;

            if (index > m_Materials.size() - 1)
            {
                m_Materials.resize(m_Materials.size() * 2);
                m_GPUMaterials.resize(m_GPUMaterials.size() * 2);
            }

            m_OccupiedMaterials.insert(index);

            material->m_AlbedoIndex    = RegisterTexture(material->GetAlbedoTexture());
            material->m_RoughnessIndex = RegisterTexture(material->GetRoughnessTexture());
            material->m_MetalnessIndex = RegisterTexture(material->GetMetalnessTexture());
            material->m_NormalIndex    = RegisterTexture(material->GetNormalTexture());

            m_GPUMaterials[index] = GPUMaterial{};
            m_Materials[index] = material;

            m_RegisteredMaterials[material->GetName()] = index;
        }

        return m_RegisteredMaterials.at(material->GetName());
    }
    uint32_t Scene::RegisterTexture(Handle<Texture> texture)
    {
        if (!m_RegisteredTextures.contains(texture->GetName()))
        {
            uint32_t index = 0U;
            while (m_OccupiedTextures.contains(index))
                index++;

            if (index > m_Textures.size()-1)
                m_Textures.resize(m_Textures.size() * 2);

            m_OccupiedTextures.insert(index);

            m_Textures[index] = texture;
            m_RegisteredTextures[texture->GetName()] = index;
            m_GlobalDescriptorChanged = true;
        }

        return m_RegisteredTextures.at(texture->GetName());
    }

    void Scene::DeregisterMatrix(uint32_t index)
    {
        m_OccupiedMatrices.erase(index);
        m_Matrices[index] = glm::mat4(1.0f);
    }
    void Scene::DeregisterMaterial(uint32_t index)
    {
        m_RegisteredMaterials.erase(m_Materials[index]->GetName());
        m_GPUMaterials[index] = GPUMaterial{};
        m_Materials[index]    = nullptr;
    }
    void Scene::DeregisterTexture(uint32_t index)
    {
        m_RegisteredTextures.erase(m_Textures[index]->GetName());
        m_Textures[index] = nullptr;
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
            m_GlobalMatricesStagingBuffer->Resize(m_GlobalMatricesBuffer->GetSize());
            m_GlobalDescriptorChanged = true;
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
                m_GlobalMatricesStagingBuffer->MapMemory(&m_Matrices[changedMatrixId], sizeof(glm::mat4));
                m_GlobalMatricesStagingBuffer->CopyTo(m_GlobalMatricesBuffer, sizeof(glm::mat4), 0U, changedMatrixId * sizeof(glm::mat4));
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
            m_GlobalMaterialsStagingBuffer->Resize(m_GlobalMaterialsBuffer->GetSize());
            m_GlobalDescriptorChanged = true;
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
                m_GlobalMaterialsStagingBuffer->MapMemory(&m_GPUMaterials[changedMaterialId], sizeof(GPUMaterial));
                m_GlobalMaterialsStagingBuffer->CopyTo(m_GlobalMaterialsBuffer, sizeof(GPUMaterial), 0U, changedMaterialId * sizeof(GPUMaterial));
            }
        }
    }
    void Scene::UpdateLightsBuffer(const std::vector<uint32_t>& changedPointLightsIDs, const std::vector<uint32_t>& changedSpotLightsIDs, const std::vector<uint32_t>& changedDirLightsIDs)
    {
        uint32_t totalPointLights = MAX_POINT_LIGHTS;
        uint32_t changedPointLights = changedPointLightsIDs.size();

        if ((float)changedPointLights / totalPointLights > POINT_LIGHTS_UPDATE_THRESHOLD)
        {
            EN_LOG("TOTAL POINT LIGHTS UPDATE");
            m_LightsStagingBuffer->MapMemory(m_GPULights.pointLights.data(), sizeof(m_GPULights.pointLights));
            m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(m_GPULights.pointLights), 0U,  (size_t)&m_GPULights.pointLights - (size_t)&m_GPULights);
        }
        else
        {
            int u = 0;
            for (const auto& changedId : changedPointLightsIDs)
            {
                m_LightsStagingBuffer->MapMemory(&m_GPULights.pointLights[changedId], sizeof(PointLight::Buffer));
                m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(PointLight::Buffer), 0U, (size_t)&m_GPULights.pointLights[changedId] - (size_t)&m_GPULights);
            }
        }

        uint32_t totalSpotLights = MAX_SPOT_LIGHTS;
        uint32_t changedSpotLights = changedSpotLightsIDs.size();

        if ((float)changedSpotLights / totalSpotLights > SPOT_LIGHTS_UPDATE_THRESHOLD)
        {
            EN_LOG("TOTAL SPOT LIGHTS UPDATE");
            m_LightsStagingBuffer->MapMemory(m_GPULights.spotLights.data(), sizeof(m_GPULights.spotLights));
            m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(m_GPULights.spotLights), 0U, (size_t)&m_GPULights.spotLights - (size_t)&m_GPULights);
        }
        else
        {
            for (const auto& changedId : changedSpotLightsIDs)
            {
                m_LightsStagingBuffer->MapMemory(&m_GPULights.spotLights[changedId], sizeof(SpotLight::Buffer));
                m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(SpotLight::Buffer), 0U, (size_t)&m_GPULights.spotLights[changedId] - (size_t)&m_GPULights);
            }
        }

        uint32_t totalDirLights = MAX_DIR_LIGHTS;
        uint32_t changedDirLights = changedDirLightsIDs.size();

        if ((float)changedDirLights / totalDirLights > DIR_LIGHTS_UPDATE_THRESHOLD)
        {
            EN_LOG("TOTAL DIR LIGHTS UPDATE");
            m_LightsStagingBuffer->MapMemory(m_GPULights.dirLights.data(), sizeof(m_GPULights.dirLights));
            m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(m_GPULights.dirLights), 0U, (size_t)&m_GPULights.dirLights - (size_t)&m_GPULights);
        }
        else
        {
            for (const auto& changedId : changedDirLightsIDs)
            {
                m_LightsStagingBuffer->MapMemory(&m_GPULights.dirLights[changedId], sizeof(DirectionalLight::Buffer));
                m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(DirectionalLight::Buffer), 0U, (size_t)&m_GPULights.dirLights[changedId] - (size_t)&m_GPULights);
            }
        }

        auto pointLightsSize = sizeof(m_GPULights.pointLights);
        auto spotLightsSize = sizeof(m_GPULights.spotLights);
        auto dirLightsSize = sizeof(m_GPULights.dirLights);
        auto allLightsSize = pointLightsSize + spotLightsSize + dirLightsSize;

        if (m_SceneLightingChanged)
        {
            EN_LOG("SCENE LIGHTING UPDATE");
            
            auto sceneLightingSize = sizeof(GPULights) - allLightsSize;
           
            m_LightsStagingBuffer->MapMemory(&m_GPULights.activePointLights, sceneLightingSize);
            m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sceneLightingSize, 0U, allLightsSize);
        }

        //EN_LOG("TOTAL LIGHTS UPDATE");
        //m_LightsStagingBuffer->MapMemory(&m_GPULights, sizeof(GPULights));
        //m_LightsStagingBuffer->CopyTo(m_LightsBuffer, sizeof(GPULights));
    }
    void Scene::UpdateGlobalDescriptor()
    {
        EN_LOG("TOTAL GLOBAL DESCRIPTOR UPDATE");

        std::vector<DescriptorInfo::ImageInfoContent> imageViews(MAX_TEXTURES, DescriptorInfo::ImageInfoContent{
            AssetManager::Get().GetWhiteNonSRGBTexture()->m_Image->GetViewHandle(),
            AssetManager::Get().GetWhiteNonSRGBTexture()->m_Sampler->GetHandle()
        });

        for (const auto& i : m_OccupiedTextures)
        {
            imageViews[i].imageView    = m_Textures[i]->m_Image->GetViewHandle();
            imageViews[i].imageSampler = m_Textures[i]->m_Sampler->GetHandle();
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
                DescriptorInfo::BufferInfo {
                    .index  = 3U,
                    .buffer = m_LightsBuffer->GetHandle(),
                    .size   = m_LightsBuffer->GetSize(),
                    .type   = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
                },
            }
        });

        m_LightsBufferDescriptorSet = MakeHandle<DescriptorSet>(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{},
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .buffer = m_LightsBuffer->GetHandle(),
                    .size = m_LightsBuffer->GetSize(),
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
                },
            }
            });

        m_LightingDescriptorSet->Update(DescriptorInfo{
            std::vector<DescriptorInfo::ImageInfo>{},
            std::vector<DescriptorInfo::BufferInfo>{
                DescriptorInfo::BufferInfo {
                    .index = 0U,
                    .buffer = m_GlobalMatricesBuffer->GetHandle(),
                    .size = m_GlobalMatricesBuffer->GetSize(),
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                },
                DescriptorInfo::BufferInfo {
                    .index = 1U,
                    .buffer = m_LightsBuffer->GetHandle(),
                    .size = m_LightsBuffer->GetSize(),
                    .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                    .stage = VK_SHADER_STAGE_VERTEX_BIT,
                },
            }
        });
    }
}