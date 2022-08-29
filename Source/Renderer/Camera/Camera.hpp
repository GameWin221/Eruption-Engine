#pragma once

#ifndef EN_CAMERA_HPP
#define EN_CAMERA_HPP

#include <Renderer/Window.hpp>
#include <Common/Helpers.hpp>

namespace en
{
	struct CameraInfo
	{
		float fov = 60.0f;

		float farPlane = 200.0f;
		float nearPlane = 0.01f;

		glm::vec2 size = glm::vec2(1920, 1080);

		glm::vec3 position = glm::vec3(0);

		float exposure = 1.0f;

		bool dynamicallyScaled = false;
	};

	class Camera
	{
	public:
		Camera(const CameraInfo& cameraInfo);

		void LookAt(glm::vec3 target);

		float m_FarPlane;
		float m_NearPlane;

		glm::vec3 m_Position;
		glm::vec2 m_Size;

		float m_Pitch;
		float m_Yaw;

		float m_Fov;

		float m_Exposure;

		bool m_DynamicallyScaled;

		const glm::mat4& GetProjMatrix() { UpdateProjMatrix(); return m_Proj; };
		const glm::mat4& GetViewMatrix() { UpdateViewMatrix(); return m_View; };

		const glm::vec3 GetFront() const { return m_Front; };
		const glm::vec3 GetUp()    const { return m_Up;    };
		const glm::vec3 GetRight() const { return m_Right; };

	private:
		// Camera
		glm::mat4 m_Proj;
		glm::mat4 m_View;

		glm::vec3 m_Front;
		glm::vec3 m_Up;
		glm::vec3 m_Right;

		const glm::vec3 m_GlobalUp = glm::vec3(0.0f, 1.0f, 0.0f);

		void UpdateProjMatrix();
		void UpdateViewMatrix();

		void UpdateVectors();
	};
}
#endif