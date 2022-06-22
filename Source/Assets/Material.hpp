#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>

#include "Asset.hpp"

namespace en
{
	class Material : public Asset
	{
		friend class AssetManager;

	public:
		Material(const std::string& name, const glm::vec3& color, const float& metalnessVal, const float& roughnessVal, const float& normalStrength, Texture* albedoTexture, Texture* roughnessTexture, Texture* normalTexture, Texture* metalnessTexture);
		~Material();

		glm::vec3 m_Color;

		float m_NormalStrength;
		float m_MetalnessVal;
		float m_RoughnessVal;

		void Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout);

		static Material* GetDefaultMaterial();

		static VkDescriptorSetLayout& GetLayout();

		void SetAlbedoTexture(Texture* texture);
		void SetRoughnessTexture(Texture* texture);
		void SetMetalnessTexture(Texture* texture);
		void SetNormalTexture(Texture* texture);

		const Texture* GetAlbedoTexture()    const { return m_Albedo;    };
		const Texture* GetRoughnessTexture() const { return m_Roughness; };
		const Texture* GetMetalnessTexture() const { return m_Metalness; };
		const Texture* GetNormalTexture()    const { return m_Normal;    };

		const std::string& GetName() const { return m_Name; };

	private:
		void UpdateDescriptorSet();
		void CreateDescriptorSet();

		void UpdateBuffer();

		bool m_UpdateQueued = false;

		std::string m_Name;

		Texture* m_Albedo;
		Texture* m_Roughness;
		Texture* m_Metalness;
		Texture* m_Normal;

		VkDescriptorSet m_DescriptorSet;

		struct MatBuffer
		{
			alignas(4) glm::vec3 color = glm::vec3(1.0f);
			alignas(4) float metalnessVal = 0.0f;
			alignas(4) float roughnessVal = 0.75f;
			alignas(4) float normalStrength = 1.0f;
		} m_MatBuffer;
	};
}

#endif