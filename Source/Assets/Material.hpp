#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>

namespace en
{
	class Material
	{
	public:
		Material(glm::vec3 color, float shininess, Texture* albedoTexture, Texture* specularTexture);
		~Material();

		glm::vec3 m_Color;

		float m_Shininess;

		Texture* m_Albedo;
		Texture* m_Specular;

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);
		void Update();

		static Material* GetDefaultMaterial();

		static VkDescriptorSetLayout& GetMatDescriptorLayout();

	private:
		void CreateDescriptorSet();
		void CreateMatBuffer();

		VkDescriptorSet m_DescriptorSet;

		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;

		struct MatBuffer
		{
			alignas(4) glm::vec3 Color = glm::vec3(1.0f);
			alignas(4) float Shininess = 32.0f;

		} m_MatBuffer;
	};
}

#endif