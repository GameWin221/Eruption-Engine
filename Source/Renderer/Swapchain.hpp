#pragma once

#ifndef EN_SWAPCHAIN_HPP
#define EN_SWAPCHAIN_HPP

#include <Renderer/Context.hpp>

namespace en
{
	class Swapchain
	{
	public:
		Swapchain(bool vSync);
		~Swapchain();

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

		std::vector<VkImage>	   m_Images;
		std::vector<VkImageView>   m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		uint32_t m_ImageIndex{};

		void ChangeLayout(uint32_t index, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd = VK_NULL_HANDLE);

		const VkFormat   const GetFormat() { return m_ImageFormat; };
		const VkExtent2D const GetExtent() { return m_Extent;	   };

	private:
		std::vector<VkImageLayout> m_CurrentLayouts;

		VkFormat   m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent;

		SwapchainSupportDetails QuerySwapchainSupport();
		VkSurfaceFormatKHR		ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR		ChooseSwapPresentMode(bool vSync, const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D				ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}

#endif