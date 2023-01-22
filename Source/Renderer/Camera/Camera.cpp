#include "Camera.hpp"

namespace en
{
	constexpr glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);

	Camera::Camera(float fov, float nearPlane, float farPlane, glm::vec3 position)
		: m_Fov(fov), m_NearPlane(nearPlane), m_FarPlane(farPlane), m_Position(position)
	{
		UpdateVectors();
	}

	void Camera::LookAt(const glm::vec3 target)
	{
		glm::vec3 diff = glm::normalize(target - m_Position);

		float dist = sqrtf((diff.x * diff.x) + (diff.z * diff.z));

		float pitch = glm::degrees(atan2f(diff.y, dist  ));
		float yaw   = glm::degrees(atan2f(diff.z, diff.x));

		m_Pitch = pitch;
		m_Yaw   = yaw;
	}

	void Camera::UpdateProjMatrix()
	{
		if (m_DynamicallyScaled)
		{
			glm::ivec2 size = en::Window::Get().GetSize();

			if (size.x <= 0 || size.y <= 0) return;

			m_Size = size;
		}

		m_Proj = glm::perspective(glm::radians(m_Fov), (float)m_Size.x / (float)m_Size.y, m_NearPlane, m_FarPlane);
		m_Proj[1][1] *= -1;
	}

	void Camera::UpdateViewMatrix()
	{
		UpdateVectors();

		m_View = glm::lookAt(m_Position, m_Position + m_Front, m_Up);
	}

	void Camera::UpdateVectors()
	{
		m_Pitch = glm::clamp(m_Pitch, -89.999f, 89.999f);

		m_Front = glm::normalize(
			glm::vec3(
				cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)),
				sin(glm::radians(m_Pitch)),
				sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))
			)
		);

		m_Right = glm::normalize(glm::cross(m_Front, WORLD_UP));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));
	}
}