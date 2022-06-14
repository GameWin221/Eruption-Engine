#include "Core/EnPch.hpp"
#include "Swapchain.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Swapchain::Swapchain(){}
	Swapchain::~Swapchain()
	{
		UseContext();

		vkDestroySwapchainKHR(ctx.m_LogicalDevice, m_Swapchain, nullptr);

		for (auto& imageView : m_ImageViews)
			vkDestroyImageView(ctx.m_LogicalDevice, imageView, nullptr);

		for (auto& framebuffer : m_Framebuffers)
			vkDestroyFramebuffer(ctx.m_LogicalDevice, framebuffer, nullptr);

		m_CurrentLayouts.clear();
	}
	
	void Swapchain::CreateSwapchain(bool vSync)
	{
		UseContext();

		SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport();

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR   presentMode = vSync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;
		VkExtent2D		   extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			imageCount = swapChainSupport.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = ctx.m_WindowSurface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1U;
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

		uint32_t queueFamilyIndices[] = { ctx.m_QueueFamilies.graphics.value(), ctx.m_QueueFamilies.present.value() };

		if (ctx.m_QueueFamilies.graphics != ctx.m_QueueFamilies.present)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2U;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0;
			createInfo.pQueueFamilyIndices = nullptr;
		}


		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		if (vkCreateSwapchainKHR(ctx.m_LogicalDevice, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
			EN_ERROR("VulkanRendererBackend::CreateSwapChain() - Failed to create swap chain!");

		vkGetSwapchainImagesKHR(ctx.m_LogicalDevice, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		vkGetSwapchainImagesKHR(ctx.m_LogicalDevice, m_Swapchain, &imageCount, m_Images.data());

		m_ImageFormat = surfaceFormat.format;
		extent = extent;

		m_ImageViews.resize(m_Images.size());
		m_CurrentLayouts.resize(m_Images.size());

		VkCommandBuffer dummy = VK_NULL_HANDLE;

		for (int i = 0; i < m_ImageViews.size(); i++)
		{
			Helpers::CreateImageView(m_Images[i], m_ImageViews[i], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			ChangeLayout(VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, i, dummy);
		}
	}
	void Swapchain::CreateSwapchainFramebuffers(VkRenderPass& inputRenderpass)
	{
		m_Framebuffers.resize(m_ImageViews.size());

		for (int i = 0; i < m_Framebuffers.size(); i++)
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType			= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass		= inputRenderpass;
			framebufferInfo.attachmentCount = 1U;
			framebufferInfo.pAttachments	= &m_ImageViews[i];
			framebufferInfo.width			= m_Extent.width;
			framebufferInfo.height			= m_Extent.height;
			framebufferInfo.layers			= 1U;

			if (vkCreateFramebuffer(Context::Get().m_LogicalDevice, &framebufferInfo, nullptr, &m_Framebuffers[i]) != VK_SUCCESS)
				EN_ERROR("VulkanRendererBackend::CreateSwapchainFramebuffers() - Failed to create framebuffers!");
		}
	}
	
	void Swapchain::ChangeLayout(VkImageLayout newLayout, int index, VkCommandBuffer& cmd)
	{
		Helpers::TransitionImageLayout(m_Images[index], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_CurrentLayouts[index], newLayout, 1U, cmd);
		m_CurrentLayouts[index] = newLayout;
	}

	SwapchainSupportDetails Swapchain::QuerySwapchainSupport()
	{
		UseContext();

		SwapchainSupportDetails details;

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx.m_PhysicalDevice, ctx.m_WindowSurface, &details.capabilities);

		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.m_PhysicalDevice, ctx.m_WindowSurface, &formatCount, nullptr);

		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(ctx.m_PhysicalDevice, ctx.m_WindowSurface, &formatCount, details.formats.data());
		}

		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.m_PhysicalDevice, ctx.m_WindowSurface, &presentModeCount, nullptr);

		if (presentModeCount != 0)
		{
			details.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(ctx.m_PhysicalDevice, ctx.m_WindowSurface, &presentModeCount, details.presentModes.data());
		}

		return details;
	}
	
	VkSurfaceFormatKHR Swapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;
		}

		return availableFormats[0];
	}
	VkExtent2D Swapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			int width, height;
			glfwGetFramebufferSize(Window::Get().m_GLFWWindow, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}
}