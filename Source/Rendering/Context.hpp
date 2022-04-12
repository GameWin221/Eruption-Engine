#pragma once

#ifndef EN_CONTEXT_HPP
#define EN_CONTEXT_HPP

#include "Window.hpp"

#include "Utility/Helpers.hpp"

struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR		capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR>	presentModes;
};

namespace en
{
	class Context
	{
	public:
		Context();
		~Context();

		// Vulkan Core
		VkInstance				 m_Instance;
		VkDevice				 m_LogicalDevice;
		VkDebugUtilsMessengerEXT m_DebugMessenger;

		// Graphics
		VkSurfaceKHR	 m_WindowSurface;
		VkPhysicalDevice m_PhysicalDevice;
		VkQueue			 m_GraphicsQueue;
		VkQueue			 m_PresentQueue;
		VkCommandPool    m_CommandPool;

		static Context& GetContext();

		Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;

	private:
		// Vulkan Initialisation
		void VKCreateInstance();
		void VKCreateDebugMessenger();
		void VKCreateWindowSurface();
		void VKPickPhysicalDevice();
		void VKCreateLogicalDevice();

		void VKCreateCommandPool();

		// Validation Layers and Extensions
		bool CheckValidationLayerSupport();
		std::vector<const char*> GetRequiredExtensions();

		// Debug Messenger
		VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo);
		void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void DestroyDebugUtilsMessengerEXT();
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		bool IsDeviceSuitable(VkPhysicalDevice& device);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice& device);
	};
}

#define UseContext() Context& ctx = Context::GetContext()

#endif