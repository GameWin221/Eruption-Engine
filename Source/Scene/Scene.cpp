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

    std::vector<SceneObject*> Scene::GetAllSceneObjects()
    {
        std::vector<SceneObject*> allSceneObjects(m_SceneObjects.size());

        for (int i = 0; auto& sceneObject : m_SceneObjects)
            allSceneObjects[i++] = sceneObject.second.get();

        return allSceneObjects;
    }
}