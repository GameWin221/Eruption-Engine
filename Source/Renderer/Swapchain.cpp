#include "Swapchain.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	Swapchain::Swapchain(bool vSync)
	{
		UseContext();

		SwapchainSupportDetails swapChainSupport = QuerySwapchainSupport();

		VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR   presentMode = ChooseSwapPresentMode(vSync, swapChainSupport.presentModes);
		VkExtent2D		   extent = ChooseSwapExtent(swapChainSupport.capabilities);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
			imageCount = swapChainSupport.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx.m_WindowSurface,
			.minImageCount = imageCount,
			.imageFormat = surfaceFormat.format,
			.imageColorSpace = surfaceFormat.colorSpace,
			.imageExtent = extent,
			.imageArrayLayers = 1U,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = swapChainSupport.capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = presentMode,
			.clipped = VK_TRUE,
			.oldSwapchain = VK_NULL_HANDLE,
		};

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

		if (vkCreateSwapchainKHR(ctx.m_LogicalDevice, &createInfo, nullptr, &m_Swapchain) != VK_SUCCESS)
			EN_ERROR("Renderer::CreateSwapChain() - Failed to create swap chain!");

		vkGetSwapchainImagesKHR(ctx.m_LogicalDevice, m_Swapchain, &imageCount, nullptr);
		m_Images.resize(imageCount);
		vkGetSwapchainImagesKHR(ctx.m_LogicalDevice, m_Swapchain, &imageCount, m_Images.data());

		m_ImageFormat = surfaceFormat.format;
		m_Extent = extent;

		m_ImageViews.resize(m_Images.size());
		m_CurrentLayouts.resize(m_Images.size());

		for (int i = 0; i < m_ImageViews.size(); i++)
		{
			Helpers::CreateImageView(m_Images[i], m_ImageViews[i], VK_IMAGE_VIEW_TYPE_2D, m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
			ChangeLayout(
				i, 
				VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, 
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 
				0U, 
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			);
		}
	}
	Swapchain::~Swapchain()
	{
		UseContext();

		for (const auto& imageView : m_ImageViews)
			vkDestroyImageView(ctx.m_LogicalDevice, imageView, nullptr);

		vkDestroySwapchainKHR(ctx.m_LogicalDevice, m_Swapchain, nullptr);

		m_CurrentLayouts.clear();
	}

	void Swapchain::ChangeLayout(uint32_t index, VkImageLayout newLayout, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage, VkCommandBuffer cmd)
	{
		Helpers::TransitionImageLayout(m_Images[index], m_ImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, m_CurrentLayouts[index], newLayout, srcAccessMask, dstAccessMask, srcStage, dstStage, 0U, 1U, 1U, cmd);
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
			glm::ivec2 size = Window::Get().GetFramebufferSize();

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(size.x),
				static_cast<uint32_t>(size.y)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}
	VkPresentModeKHR Swapchain::ChooseSwapPresentMode(bool vSync, const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		if (vSync)
		{
			return VK_PRESENT_MODE_FIFO_KHR;
		}
		else
		{
			bool supportsMailbox = false;
			for (const auto& mode : availablePresentModes)
			{
				if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					supportsMailbox = true;
					break;
				}
			}

			if (supportsMailbox)
				return VK_PRESENT_MODE_MAILBOX_KHR;
			else
				return VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}
}