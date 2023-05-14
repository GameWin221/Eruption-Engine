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
		friend class Scene;

	public:
		Material(const std::string& name, const glm::vec3 color, const float metalnessVal, const float roughnessVal, const float normalStrength, Handle<Texture> albedoTexture, Handle<Texture> roughnessTexture, Handle<Texture> normalTexture, Handle<Texture> metalnessTexture)
			: m_Name(name), m_Color(color), m_MetalnessVal(metalnessVal), m_RoughnessVal(roughnessVal), m_NormalStrength(normalStrength), m_Albedo(albedoTexture), m_Roughness(roughnessTexture), m_Metalness(metalnessTexture), m_Normal(normalTexture), Asset{ AssetType::Material } { }

		void SetColor		  (glm::vec3 color);
		void SetNormalStrength(float normalStrength);
		void SetMetalness	  (float metalness);
		void SetRoughness	  (float roughness);

		void SetAlbedoTexture	(Handle<Texture> texture);
		void SetRoughnessTexture(Handle<Texture> texture);
		void SetMetalnessTexture(Handle<Texture> texture);
		void SetNormalTexture	(Handle<Texture> texture);

		const Handle<Texture> GetAlbedoTexture()    const { return m_Albedo;    };
		const Handle<Texture> GetRoughnessTexture() const { return m_Roughness; };
		const Handle<Texture> GetMetalnessTexture() const { return m_Metalness; };
		const Handle<Texture> GetNormalTexture()    const { return m_Normal;    };

		const uint32_t GetAlbedoIndex()    const { return m_AlbedoIndex;    };
		const uint32_t GetRoughnessIndex() const { return m_RoughnessIndex; };
		const uint32_t GetMetalnessIndex() const { return m_MetalnessIndex; };
		const uint32_t GetNormalIndex()    const { return m_NormalIndex;    };

		const glm::vec3 GetColor()		const { return m_Color;			 };
		const float GetNormalStrength() const { return m_NormalStrength; };
		const float GetMetalness()		const { return m_MetalnessVal;   };
		const float GetRoughness()		const { return m_RoughnessVal;   };

		const std::string& GetName() const { return m_Name; };
	private:
		bool m_Changed = true;

		glm::vec3 m_Color;

		float m_NormalStrength;
		float m_MetalnessVal;
		float m_RoughnessVal;

		std::string m_Name;

		Handle<Texture> m_Albedo;
		Handle<Texture> m_Roughness;
		Handle<Texture> m_Metalness;
		Handle<Texture> m_Normal;

		uint32_t m_AlbedoIndex{};
		uint32_t m_RoughnessIndex{};
		uint32_t m_MetalnessIndex{};
		uint32_t m_NormalIndex{};
	};
}

#endif