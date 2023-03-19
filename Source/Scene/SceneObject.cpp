#include "SceneObject.hpp"

namespace en
{
    void SceneObject::SetPosition(const glm::vec3& position)
    {
        if (!m_TransformChanged && position != m_Position)
            m_TransformChanged = true;

        m_Position = position;
    }
    void SceneObject::SetRotation(const glm::vec3& rotation)
    {
        if (!m_TransformChanged && rotation != m_Rotation)
            m_TransformChanged = true;

        m_Rotation = rotation;
    }
    void SceneObject::SetScale(const glm::vec3& scale)
    {
        if (!m_TransformChanged && scale != m_Scale)
            m_TransformChanged = true;

        m_Scale = scale;
    }
}