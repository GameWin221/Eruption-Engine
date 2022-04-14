#pragma once

#ifndef EN_TEXTURE_HPP
#define EN_TEXTURE_HPP

#include "../../EruptionEngine.ini"

#include "Utility/Helpers.hpp"

#include <Rendering/Context.hpp>

#include <stb_image.h>

namespace en
{
	class Texture
	{
	public:
		Texture(std::string texturePath);
		Texture(stbi_uc* pixelData, glm::uvec2 size);
		~Texture();

		VkImage		m_Image;
		VkImageView m_ImageView;
		VkSampler   m_ImageSampler;
		VkDeviceMemory m_ImageMemory;

		static Texture* GetWhiteTexture();

		const glm::ivec2& GetSize()     const { return this->m_Size; };
		const std::string& GetFilePath() const { return this->m_FilePath; };

	private:
		void CreateImageSampler();
		void CopyBufferToImage(const VkBuffer& srcBuffer);

		glm::ivec2  m_Size;
		std::string m_FilePath;
		int		    m_Channels;
	};
}

#endif