#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>

namespace en
{
	class Material
	{
		friend class AssetManager;

	public:
		Material(std::string name, glm::vec3 color, float shininess, float normalStrength, Texture* albedoTexture, Texture* specularTexture, Texture* normalTexture);
		~Material();

		glm::vec3 m_Color;

		float m_Shininess;
		float m_NormalStrength;

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);

		static Material* GetDefaultMaterial();

		static VkDescriptorSetLayout& GetLayout();

		void SetAlbedoTexture(Texture* texture);
		void SetSpecularTexture(Texture* texture);
		void SetNormalTexture(Texture* texture);

		const Texture* GetAlbedoTexture()   const { return m_Albedo;   };
		const Texture* GetSpecularTexture() const { return m_Specular; };
		const Texture* GetNormalTexture()   const { return m_Normal;   };

		const std::string& GetName() const { return m_Name; };

	private:
		void UpdateDescriptorSet();
		void CreateDescriptorSet();
		void CreateMatBuffer();

		Texture* m_Albedo;
		Texture* m_Specular;
		Texture* m_Normal;

		VkDescriptorSet m_DescriptorSet;

		VkBuffer m_Buffer;
		VkDeviceMemory m_BufferMemory;

		bool m_UpdateQueued = false;

		std::string m_Name;

		struct MatBuffer
		{
			alignas(4) glm::vec3 color = glm::vec3(1.0f);
			alignas(4) float shininess = 32.0f;
			alignas(4) float normalStrength = 0.5f;

		} m_MatBuffer;

		bool m_WillBeDeleted = false;
	};
}

#endif