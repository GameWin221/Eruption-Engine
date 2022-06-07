#include <Core/EnPch.hpp>
#include "Scene.hpp"

namespace en
{
    SceneObject* g_EmptySceneObject;

    SceneObject* Scene::CreateSceneObject(std::string name, Mesh* mesh)
    {
        if (m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::CreateSceneObject() - Failed to create a SceneObject because a SceneObject called \"" + name + "\" already exists!");
            return nullptr;
        }

        m_SceneObjects[name] = std::make_unique<SceneObject>(mesh, name);

        EN_LOG("Created a SceneObject called \"" + name + "\"");

        return m_SceneObjects.at(name).get();
    }

    void Scene::DeleteSceneObject(std::string name)
    {
        if (!m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::DeleteSceneObject() - Failed to delete a SceneObject because a SceneObject called \"" + name + "\" doesn't exist!");
            return;
        }

        EN_LOG("Deleted a SceneObject called \"" + name + "\"");

        m_SceneObjects.erase(name);
    }

    SceneObject* Scene::GetSceneObject(std::string name)
    {
        if (!m_SceneObjects.contains(name))
        {
            EN_WARN("Scene::GetSceneObject() - Failed to find a SceneObject named " + name + "!");

            return nullptr;
        }

        return m_SceneObjects.at(name).get();
    }

    void Scene::RenameSceneObject(std::string oldName, std::string newName)
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

    PointLight* Scene::CreatePointLight(glm::vec3 position, glm::vec3 color, float intensity, float radius)
    {
        if (m_PointLights.size() + 1 >= MAX_POINT_LIGHTS)
        {
            EN_WARN("Failed to create a new PointLight because you've reached the MAX_POINT_LIGHTS limit (" + std::to_string(MAX_POINT_LIGHTS) + ")!")
            return nullptr;
        }

        m_PointLights.emplace_back(position, color, intensity, radius);

        return &m_PointLights.back();
    }
    void Scene::DeletePointLight(uint32_t index)
    {
        m_PointLights.erase(m_PointLights.begin() + index);
    }

    Spotlight* Scene::CreateSpotlight(glm::vec3 position, glm::vec3 direction, glm::vec3 color, float innerCutoff, float outerCutoff, float range, float intensity)
    {
        if (m_Spotlights.size() + 1 >= MAX_SPOT_LIGHTS)
        {
            EN_WARN("Failed to create a new Spotlight because you've reached the MAX_SPOT_LIGHTS limit (" + std::to_string(MAX_SPOT_LIGHTS) + ")!")
            return nullptr;
        }

        m_Spotlights.emplace_back(position, direction, color, innerCutoff, outerCutoff, range, intensity);

        return &m_Spotlights.back();
    }
    void Scene::DeleteSpotlight(uint32_t index)
    {
        m_Spotlights.erase(m_Spotlights.begin() + index);
    }

    DirectionalLight* Scene::CreateDirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity)
    {
        if (m_DirectionalLights.size() + 1 >= MAX_DIR_LIGHTS)
        {
            EN_WARN("Failed to create a new DirectionalLight because you've reached the MAX_DIR_LIGHTS limit (" + std::to_string(MAX_DIR_LIGHTS) + ")!")
            return nullptr;
        }

        m_DirectionalLights.emplace_back(direction, color, intensity);

        return &m_DirectionalLights.back();
    }
    void Scene::DeleteDirectionalLight(uint32_t index)
    {
        m_DirectionalLights.erase(m_DirectionalLights.begin() + index);
    }

    std::vector<SceneObject*> Scene::GetAllSceneObjects()
    {
        std::vector<SceneObject*> allSceneObjects(m_SceneObjects.size());

        for (int i = 0; auto& sceneObject : m_SceneObjects)
            allSceneObjects[i++] = sceneObject.second.get();

        return allSceneObjects;
    }
}