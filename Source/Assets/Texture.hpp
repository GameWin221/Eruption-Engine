#pragma once

#ifndef EN_TEXTURE_HPP
#define EN_TEXTURE_HPP

#include "../../EruptionEngine.ini"

#include <stb_image.h>

#include <Renderer/Buffers/MemoryBuffer.hpp>
#include <Common/Helpers.hpp>

namespace en
{
	class Texture
	{
		friend class AssetManager;

	public:
		Texture(std::string texturePath, std::string name, VkFormat format, bool flipTexture = false);
		Texture(stbi_uc* pixelData, std::string name, VkFormat format, glm::uvec2 size);
		~Texture();

		VkImage		m_Image;
		VkImageView m_ImageView;
		VkSampler   m_ImageSampler;
		VkFormat    m_ImageFormat;
		VkDeviceMemory m_ImageMemory;

		static Texture* GetWhiteSRGBTexture();
		static Texture* GetGreyNonSRGBTexture();
		static Texture* GetNormalTexture();

		const glm::ivec2&  GetSize()     const { return m_Size;     };
		const std::string& GetFilePath() const { return m_FilePath; };
		const std::string& GetName()     const { return m_Name;	    };

	private:
		void CreateImageSampler();

		std::string m_Name;

		glm::ivec2  m_Size;
		std::string m_FilePath;
		int		    m_Channels;
	};
}

#endif