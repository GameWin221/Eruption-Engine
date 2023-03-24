#include "EditorImageAtlas.hpp"

namespace en
{
	EditorImageAtlas::EditorImageAtlas(const std::string& atlasPath, VkExtent2D size)
	{
		m_Texture = MakeHandle<Texture>(atlasPath, "EditorImageAtlas", VK_FORMAT_R8G8B8A8_SRGB, false, false);

		m_ImageUVSize = glm::vec2(1.0f) / glm::vec2(size.width, size.height);

		m_ImGuiDescriptorSet = ImGui_ImplVulkan_AddTexture(
			m_Texture->m_ImageSampler,
			m_Texture->m_Image->GetViewHandle(),
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);
	}

	ImageUVs EditorImageAtlas::GetImageUVs(uint32_t x, uint32_t y)
	{
		const ImVec2 uv0(x	   * m_ImageUVSize.x, y		* m_ImageUVSize.y);
		const ImVec2 uv1(uv0.x + m_ImageUVSize.x, uv0.y + m_ImageUVSize.y);

		return { uv0, uv1 };
	}
}