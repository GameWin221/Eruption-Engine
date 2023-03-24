#pragma once

#ifndef EN_EditorImageAtlas_HPP
#define EN_EditorImageAtlas_HPP

#include <Assets/Texture.hpp>

#include <imgui.h>
#include <imgui_impl_vulkan.h>

namespace en
{
	struct ImageUVs
	{
		ImVec2 uv0{};
		ImVec2 uv1{};
	};

	class EditorImageAtlas
	{
	public:
		EditorImageAtlas(const std::string& atlasPath, VkExtent2D size);

		ImageUVs GetImageUVs(uint32_t x, uint32_t y);

		VkDescriptorSet m_ImGuiDescriptorSet;

	private:
		Handle<Texture> m_Texture;

		glm::vec2 m_ImageUVSize;
	};
}

#endif