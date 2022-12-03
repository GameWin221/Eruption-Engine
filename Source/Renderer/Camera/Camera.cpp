#include <Core/EnPch.hpp>
#include "Camera.hpp"

namespace en
{
	constexpr glm::vec3 WORLD_UP(0.0f, 1.0f, 0.0f);

	Camera::Camera(const CameraInfo& cameraInfo)
	{
		m_FarPlane  = cameraInfo.farPlane;
		m_NearPlane = cameraInfo.nearPlane;
		m_Position  = cameraInfo.position;
		m_Size      = cameraInfo.size;
		m_Fov       = cameraInfo.fov;

		m_Pitch = 0.0f;
		m_Yaw   = 0.0f;

		m_DynamicallyScaled = cameraInfo.dynamicallyScaled;

		m_Exposure = cameraInfo.exposure;

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
			int wX, wY;
			glfwGetWindowSize(en::Window::Get().m_GLFWWindow, &wX, &wY);

			if (wX <= 0 || wY <= 0) return;

			m_Size = glm::vec2(wX, wY);
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