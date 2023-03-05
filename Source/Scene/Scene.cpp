#include "Scene.hpp"

namespace en
{
    Handle<SceneObject> g_EmptySceneObject;

    Handle<SceneObject> Scene::CreateSceneObject(const std::string& name, Handle<Mesh> mesh)
    {
        if (m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::CreateSceneObject() - Failed to create a SceneObject because a SceneObject called \"" + name + "\" already exists!");
            return nullptr;
        }

        m_SceneObjects[name] = MakeHandle<SceneObject>(mesh, name);

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

    Scene::Scene()
    {
        m_SceneObjects.reserve(64);
        m_PointLights.reserve(64);
        m_SpotLights.reserve(64);
        m_DirectionalLights.reserve(64);

        m_MainCamera = MakeHandle<Camera>();
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
        for (auto& [name, sceneObject] : m_SceneObjects)
            sceneObject->UpdateModelMatrix();
    }

    std::vector<Handle<SceneObject>> Scene::GetAllSceneObjects()
    {
        std::vector<Handle<SceneObject>> allSceneObjects(m_SceneObjects.size());

        for (uint32_t i{}; const auto & [name, sceneObject] : m_SceneObjects)
            allSceneObjects[i++] = sceneObject;

        return allSceneObjects;
    }
}