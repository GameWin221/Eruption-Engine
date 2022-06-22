#pragma once

#ifndef EN_TEXTURE_HPP
#define EN_TEXTURE_HPP

#include "../../EruptionEngine.ini"

#include "Asset.hpp"

#include <stb_image.h>

#include <Renderer/Image.hpp>
#include <Renderer/Context.hpp>

namespace en
{
	class Texture : public Asset
	{
		friend class AssetManager;

	public:
		Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture = false, bool useMipMaps = true);
		Texture(stbi_uc* pixels, std::string name, VkFormat format, VkExtent2D size, bool useMipMaps = true);
		~Texture();

		std::unique_ptr<Image> m_Image;

		VkSampler m_ImageSampler;

		static Texture* GetWhiteSRGBTexture();
		static Texture* GetWhiteNonSRGBTexture();

		const std::string& GetFilePath() const { return m_FilePath; };
		const std::string& GetName()     const { return m_Name;	    };

	private:
		std::string m_Name;
		std::string m_FilePath;
	};
}

#endif