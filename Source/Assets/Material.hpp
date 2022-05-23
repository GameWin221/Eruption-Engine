#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>
#include <Renderer/Buffers/MemoryBuffer.hpp>

namespace en
{
	class Material
	{
		friend class AssetManager;

	public:
		Material(std::string name, glm::vec3 color, float metalness, float normalStrength, Texture* albedoTexture, Texture* roughnessTexture, Texture* normalTexture);
		~Material();

		glm::vec3 m_Color;

		float m_NormalStrength;
		float m_Metalness;

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);

		static Material* GetDefaultMaterial();

		static VkDescriptorSetLayout& GetLayout();

		void SetAlbedoTexture(Texture* texture);
		void SetRoughnessTexture(Texture* texture);
		void SetNormalTexture(Texture* texture);

		const Texture* GetAlbedoTexture()    const { return m_Albedo;    };
		const Texture* GetRoughnessTexture() const { return m_Roughness; };
		const Texture* GetNormalTexture()    const { return m_Normal;    };

		const std::string& GetName() const { return m_Name; };

	private:
		void UpdateDescriptorSet();
		void CreateDescriptorSet();

		bool m_UpdateQueued = false;

		std::string m_Name;

		Texture* m_Albedo;
		Texture* m_Roughness;
		Texture* m_Normal;

		std::unique_ptr<MemoryBuffer> m_Buffer;

		VkDescriptorSet m_DescriptorSet;

		struct MatBuffer
		{
			alignas(4) glm::vec3 color = glm::vec3(1.0f);
			alignas(4) float metalness = 0.0f;
			alignas(4) float normalStrength = 1.0f;
		} m_MatBuffer;
	};
}

#endif