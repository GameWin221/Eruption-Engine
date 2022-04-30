#include <Core/EnPch.hpp>
#include "Camera.hpp"

namespace en
{
	// Camera
	Camera::Camera(CameraInfo& cameraInfo)
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

	void Camera::LookAt(glm::vec3 target)
	{
		glm::vec3 diff = glm::normalize(target - m_Position);

		float dist = sqrtf((diff.x * diff.x) + (diff.z * diff.z));

		float pitch = glm::degrees(atan2f(diff.y, dist  ));
		float yaw   = glm::degrees(atan2f(diff.z, diff.x));

		m_Pitch = pitch;
		m_Yaw   = yaw;
	}

	void Camera::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, CameraMatricesBuffer* cameraMatricesBuffer)
	{
		cameraMatricesBuffer->m_Matrices.proj = GetProjMatrix();
		cameraMatricesBuffer->m_Matrices.view = GetViewMatrix();
		cameraMatricesBuffer->Bind(cmd, layout);
	}

	void Camera::UpdateProjMatrix()
	{
		if (m_DynamicallyScaled)
		{
			int wX, wY;
			glfwGetWindowSize(en::Window::GetMainWindow().m_GLFWWindow, &wX, &wY);

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
		// Clamping Yaw
		if     (m_Yaw >  180.0f) m_Yaw = -180.0f+(m_Yaw-180.0f);
		else if(m_Yaw < -180.0f) m_Yaw =  180.0f+(m_Yaw+180.0f);

		// Clamping Pitch
		if		(m_Pitch >  89.999f) m_Pitch =  89.999f;
		else if (m_Pitch < -89.999f) m_Pitch = -89.999f;

		m_Front = glm::normalize(
			glm::vec3(
				cos(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch)),
				sin(glm::radians(m_Pitch)),
				sin(glm::radians(m_Yaw)) * cos(glm::radians(m_Pitch))
			)
		);

		m_Right = glm::normalize(glm::cross(m_Front, m_GlobalUp));
		m_Up = glm::normalize(glm::cross(m_Right, m_Front));
	}
}