#pragma once

#ifndef EN_EditorImageAtlas_HPP
#define EN_EditorImageAtlas_HPP

#include <Assets/Texture.hpp>

#include <imgui.h>

namespace en
{
	struct ImageUVs
	{
		ImVec2 uv0;
		ImVec2 uv1;
	};

	class EditorImageAtlas
	{
	public:
		EditorImageAtlas(const std::string& atlasPath, const uint32_t& x, const uint32_t& y);
		~EditorImageAtlas();

		ImageUVs GetImageUVs(const uint32_t& x, const uint32_t& y);

		VkDescriptorSet m_DescriptorSet;

	private:
		std::unique_ptr<Texture> m_Texture;

		VkDescriptorSetLayout m_DescriptorLayout;
		VkDescriptorPool m_DescriptorPool;

		void CreateDescriptorPool();
		void CreateDescriptorSet();

		glm::vec2 m_ImageUVSize;
	};
}

#endif