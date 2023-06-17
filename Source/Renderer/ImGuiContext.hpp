#pragma once

#ifndef EN_IMGUICONTEXT_HPP
#define EN_IMGUICONTEXT_HPP

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

#include <Renderer/Window.hpp>
#include <Renderer/Context.hpp>

namespace en
{
	class ImGuiContext
	{
	public:
		ImGuiContext(VkFormat imageFormat, VkExtent2D imageExtent, const std::vector<VkImageView>& imageViews);
		~ImGuiContext();

		void UpdateFramebuffers(VkExtent2D imageExtent, const std::vector<VkImageView>& imageViews);

		void Render(VkCommandBuffer cmd, uint32_t imageIndex);

	private:
		VkRenderPass m_RenderPass{};
		VkExtent2D m_Extent{};

		std::vector<VkFramebuffer> m_Framebuffers{};
	};
}

#endif