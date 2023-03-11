#pragma once

#ifndef EN_MATERIAL_HPP
#define EN_MATERIAL_HPP

#include <Assets/Texture.hpp>
#include <Renderer/DescriptorSet.hpp>
#include <Renderer/DescriptorAllocator.hpp>

#include "Asset.hpp"

namespace en
{
	class Material : public Asset
	{
		friend class AssetManager;

	public:
		Material(const std::string& name, const glm::vec3 color, const float metalnessVal, const float roughnessVal, const float normalStrength, Handle<Texture> albedoTexture, Handle<Texture> roughnessTexture, Handle<Texture> normalTexture, Handle<Texture> metalnessTexture);
		~Material();

		void SetColor(glm::vec3 color);
		void SetNormalStrength(float normalStrength);
		void SetMetalness(float metalness);
		void SetRoughness(float roughness);

		void SetAlbedoTexture(Handle<Texture> texture);
		void SetRoughnessTexture(Handle<Texture> texture);
		void SetMetalnessTexture(Handle<Texture> texture);
		void SetNormalTexture(Handle<Texture> texture);

		const Handle<Texture> GetAlbedoTexture()    const { return m_Albedo;    };
		const Handle<Texture> GetRoughnessTexture() const { return m_Roughness; };
		const Handle<Texture> GetMetalnessTexture() const { return m_Metalness; };
		const Handle<Texture> GetNormalTexture()    const { return m_Normal;    };

		const glm::vec3 GetColor() const { return m_Color; };
		const float GetNormalStrength() const { return m_NormalStrength; };
		const float GetMetalness() const { return m_MetalnessVal; };
		const float GetRoughness() const { return m_RoughnessVal; };

		const std::string& GetName() const { return m_Name; };

		const void* GetMatBufferPtr() const { return &m_MatBuffer; };

		static VkDescriptorSetLayout GetLayout();

		const Handle<DescriptorSet> GetDescriptorSet() const { return m_DescriptorSet; };

	private:
		void Update();

		DescriptorInfo MakeDescriptorInfo();

		bool m_UpdateQueued = false;

		glm::vec3 m_Color;

		float m_NormalStrength;
		float m_MetalnessVal;
		float m_RoughnessVal;

		std::string m_Name;

		Handle<Texture> m_Albedo;
		Handle<Texture> m_Roughness;
		Handle<Texture> m_Metalness;
		Handle<Texture> m_Normal;

		Handle<DescriptorSet> m_DescriptorSet;

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