#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>

namespace en
{
	class Material
	{
	public:
		Material(std::string name, glm::vec3 color, float shininess, float normalStrength, Texture* albedoTexture, Texture* specularTexture, Texture* normalTexture);
		~Material();

		glm::vec3 m_Color;

		float m_Shininess;
		float m_NormalStrength;

		Texture* m_Albedo;
		Texture* m_Specular;
		Texture* m_Normal;

		std::string m_Name;

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);
		void Update();

		static Material* GetDefaultMaterial();

		static VkDescriptorSetLayout& GetLayout();

	private:
		void CreateDescriptorSet();
		void CreateMatBuffer();

		VkDescriptorSet m_DescriptorSet;

		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;

		struct MatBuffer
		{
			alignas(4) glm::vec3 color = glm::vec3(1.0f);
			alignas(4) float shininess = 32.0f;
			alignas(4) float normalStrength = 0.5f;

		} m_MatBuffer;
	};
}

#endif