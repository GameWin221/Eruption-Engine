#pragma once

#ifndef EN_CONTEXT_HPP
#define EN_CONTEXT_HPP

#include <Renderer/Window.hpp>
#include <Core/Types.hpp>

#include <VMA.h>

#include <set>
#include <array>
#include <vector>

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

		VkInstance				 m_Instance;
		VkDevice				 m_LogicalDevice;
		VkDebugUtilsMessengerEXT m_DebugMessenger;

		VkSurfaceKHR	 m_WindowSurface;
		VkPhysicalDevice m_PhysicalDevice;

		VkCommandPool m_GraphicsCommandPool;
		VkCommandPool m_TransferCommandPool;

		VkQueue	m_GraphicsQueue;
		VkQueue	m_TransferQueue;
		VkQueue	m_PresentQueue;

		VmaAllocator m_Allocator;

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

		const std::string& GetPhysicalDeviceName() const { return m_PhysicalDeviceName; };

	private:
		void CreateInstance();
		void CreateDebugMessenger();
		void CreateWindowSurface();
		void PickPhysicalDevice();
		void FindQueueFamilies(VkPhysicalDevice& device);
		void CreateLogicalDevice();
		void InitVMA();
		void CreateCommandPool();

		std::string m_PhysicalDeviceName;

		bool AreValidationLayerSupported();
		std::vector<const char*> GetRequiredExtensions();

		VkResult CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo);
		VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerCreateInfo();

		void DestroyDebugUtilsMessengerEXT();
		static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);

		bool IsDeviceSuitable(VkPhysicalDevice& device);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice& device);
	};
}

#define UseContext() Context& ctx = Context::Get()

#endif