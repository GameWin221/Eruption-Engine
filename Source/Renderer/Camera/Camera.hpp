#pragma once

#ifndef EN_CAMERA_HPP
#define EN_CAMERA_HPP

#include <Renderer/Window.hpp>
#include <Common/Helpers.hpp>

#include <gtc/matrix_transform.hpp>

namespace en
{
	class Camera
	{
	public:
		Camera(float fov = 60.0f, float nearPlane = 0.01f, float farPlane = 200.0f, glm::vec3 position = glm::vec3(0.0f));

		void LookAt(const glm::vec3 target);

		float m_FarPlane;
		float m_NearPlane;

		glm::vec3 m_Position;
		glm::vec2 m_Size = glm::vec3(0.0f);

		float m_Pitch = 0.0f;
		float m_Yaw = 0.0f;

		float m_Fov;

		float m_Exposure = 1.0f;

		bool m_DynamicallyScaled = true;

		const glm::mat4& GetProjMatrix() { UpdateProjMatrix(); return m_Proj; };
		const glm::mat4& GetViewMatrix() { UpdateViewMatrix(); return m_View; };

		const glm::vec3& GetFront() const { return m_Front; };
		const glm::vec3& GetUp()    const { return m_Up;    };
		const glm::vec3& GetRight() const { return m_Right; };

	private:
		glm::mat4 m_Proj;
		glm::mat4 m_View;

		glm::vec3 m_Front;
		glm::vec3 m_Up;
		glm::vec3 m_Right;

		void UpdateProjMatrix();
		void UpdateViewMatrix();

		void UpdateVectors();
	};
}
#endif