#pragma once

#ifndef EN_CONTEXT_HPP
#define EN_CONTEXT_HPP

#include <Renderer/Window.hpp>

namespace en
{
	struct SwapchainSupportDetails
	{
		VkSurfaceCapabilitiesKHR		capabilities{};
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR>	presentModes;
	};

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
		VkCommandPool    m_CommandPool;

		VkCommandPool m_TransferCommandPool;

		VkQueue	m_GraphicsQueue;
		VkQueue	m_TransferQueue;
		VkQueue	m_PresentQueue;

		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics;
			std::optional<uint32_t> transfer;
			std::optional<uint32_t> present;

			bool IsComplete()
			{
				return graphics.has_value() && present.has_value() && transfer.has_value();
			}
		} m_QueueFamilies;

		static Context& Get();

		Context(const Context&) = delete;
		Context& operator=(const Context&) = delete;

	private:
		// Vulkan Initialisation
		void CreateInstance();
		void CreateDebugMessenger();
		void CreateWindowSurface();
		void PickPhysicalDevice();
		void FindQueueFamilies(VkPhysicalDevice& device);
		void CreateLogicalDevice();
		void CreateCommandPool();

		// Validation Layers and Extensions
		bool AreValidationLayerSupported();
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

#define UseContext() Context& ctx = Context::Get()

#endif