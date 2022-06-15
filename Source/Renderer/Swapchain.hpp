#pragma once

#ifndef EN_SWAPCHAIN_HPP
#define EN_SWAPCHAIN_HPP

#include <Renderer/Context.hpp>

namespace en
{
	class Swapchain
	{
	public:
		Swapchain();
		~Swapchain();

		VkSwapchainKHR m_Swapchain;

		std::vector<VkImage>	   m_Images;
		std::vector<VkImageView>   m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		void ChangeLayout(VkImageLayout newLayout, int index, VkCommandBuffer& cmd);

		void CreateSwapchain(bool vSync);
		void CreateSwapchainFramebuffers(VkRenderPass& inputRenderpass);

		const VkFormat&   const GetFormat() { return m_ImageFormat; };
		const VkExtent2D& const GetExtent() { return m_Extent;	    };

	private:
		std::vector<VkImageLayout> m_CurrentLayouts;

		VkFormat   m_ImageFormat;
		VkExtent2D m_Extent;

		SwapchainSupportDetails QuerySwapchainSupport();
		VkSurfaceFormatKHR		ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkExtent2D				ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}

#endif