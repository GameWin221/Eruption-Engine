#pragma once

#ifndef EN_SWAPCHAIN_HPP
#define EN_SWAPCHAIN_HPP

#include <Renderer/Context.hpp>

namespace en
{
	class Swapchain
	{
	public:
		~Swapchain();

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;

		std::vector<VkImage>	   m_Images;
		std::vector<VkImageView>   m_ImageViews;
		std::vector<VkFramebuffer> m_Framebuffers;

		//void ChangeLayout(const VkImageLayout& newLayout, const int& index, const VkCommandBuffer& cmd = VK_NULL_HANDLE);
		void ChangeLayout(const int& index, const VkImageLayout& newLayout, const VkAccessFlags& srcAccessMask, const VkAccessFlags& dstAccessMask, const VkPipelineStageFlags& srcStage, const VkPipelineStageFlags& dstStage, const VkCommandBuffer& cmd = VK_NULL_HANDLE);

		void CreateSwapchain(const bool& vSync);
		void CreateSwapchainFramebuffers(const VkRenderPass& inputRenderpass);

		const VkFormat&   const GetFormat() { return m_ImageFormat; };
		const VkExtent2D& const GetExtent() { return m_Extent;	    };

	private:
		std::vector<VkImageLayout> m_CurrentLayouts;

		VkFormat   m_ImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_Extent;

		SwapchainSupportDetails QuerySwapchainSupport();
		VkSurfaceFormatKHR		ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR		ChooseSwapPresentMode(const bool& vSync, const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D				ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
	};
}

#endif