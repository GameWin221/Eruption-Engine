#include "Context.hpp"

en::Context* g_CurrentContext = nullptr;

constexpr std::array<const char*, 1> validationLayers {
	"VK_LAYER_KHRONOS_validation"
};
constexpr std::array<const char*, 2> deviceExtensions {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
};
constexpr VkPhysicalDeviceVulkan13Features deviceFeaturesVK1_3{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
	.pNext = (void*)nullptr,
	.dynamicRendering = VK_TRUE,
};
constexpr VkPhysicalDeviceVulkan12Features deviceFeaturesVK1_2{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
	.pNext = (void*)&deviceFeaturesVK1_3,
	.descriptorBindingUpdateUnusedWhilePending = VK_TRUE,
};
constexpr VkPhysicalDeviceVulkan11Features deviceFeaturesVK1_1{
	.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
	.pNext = (void*)&deviceFeaturesVK1_2,
};
constexpr VkPhysicalDeviceFeatures deviceFeatures {
	.samplerAnisotropy = VK_TRUE,
};

#if defined(_DEBUG)
constexpr bool enableValidationLayers = true;
#else
constexpr bool enableValidationLayers = false;
#endif

namespace en
{
	Context::Context()
	{
		g_CurrentContext = this;
		
		CreateInstance();
		CreateDebugMessenger();
		CreateWindowSurface();
		PickPhysicalDevice();
		CreateLogicalDevice();
		InitVMA();
		CreateCommandPool();
		CreateDescriptorAllocator();

		EN_SUCCESS("Created the Vulkan context");
	}
	Context::~Context()
	{
		m_DescriptorAllocator.reset();

		vmaDestroyAllocator(m_Allocator);

		vkDeviceWaitIdle(m_LogicalDevice);

		vkDestroyCommandPool(m_LogicalDevice, m_GraphicsCommandPool, nullptr);
		vkDestroyCommandPool(m_LogicalDevice, m_TransferCommandPool, nullptr);

		vkDestroyDevice(m_LogicalDevice, nullptr);

	    if constexpr (enableValidationLayers)
			DestroyDebugUtilsMessengerEXT();

		vkDestroySurfaceKHR(m_Instance, m_WindowSurface, nullptr);
		vkDestroyInstance(m_Instance, nullptr);
		
		glfwTerminate();

		EN_LOG("Destroyed the Vulkan context")
	}

	Context& Context::Get() 
	{
		if (!g_CurrentContext)
			EN_ERROR("Context::Get() - g_CurrentContext was a nullptr!");

		return *g_CurrentContext;
	}

	void Context::CreateInstance()
	{
		if constexpr (enableValidationLayers)
			if(!AreValidationLayerSupported())
				EN_ERROR("Context::VKCreateInstance() - Validation layers requested, but not available!");

		constexpr VkApplicationInfo appInfo{
			.sType				= VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName	= "Eruption Renderer",
			.applicationVersion = VK_MAKE_API_VERSION(1, 0, 0, 0),
			.pEngineName		= "Eruption Engine",
			.engineVersion		= VK_MAKE_API_VERSION(1, 0, 0, 0),
			.apiVersion			= VK_API_VERSION_1_3
		};

		auto extensions = GetRequiredExtensions();

		VkInstanceCreateInfo createInfo {
			.sType				     = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo	     = &appInfo,
			.enabledExtensionCount   = static_cast<uint32_t>(extensions.size()),
			.ppEnabledExtensionNames = extensions.data()
		};

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		if constexpr (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			debugCreateInfo = CreateDebugMessengerCreateInfo();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}

		if (vkCreateInstance(&createInfo, nullptr, &m_Instance) != VK_SUCCESS)
			EN_ERROR("Context::VKCreateInstance() - Failed to create instance!");
	}
	void Context::CreateDebugMessenger()
	{
		if constexpr (!enableValidationLayers) return;

		VkDebugUtilsMessengerCreateInfoEXT createInfo = CreateDebugMessengerCreateInfo();

		if (CreateDebugUtilsMessengerEXT(&createInfo) != VK_SUCCESS)
			EN_ERROR("Context::VKCreateDebugMessenger() - Failed to set up debug messenger!");
	}
	void Context::CreateWindowSurface()
	{
		if (glfwCreateWindowSurface(m_Instance, Window::Get().GetNativeHandle(), nullptr, &m_WindowSurface) != VK_SUCCESS)
			EN_ERROR("Context::VKCreateWindowSurface() - Failed to create window surface!");
	}
	void Context::PickPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

		if (deviceCount == 0)
			EN_ERROR("Context::VKPickPhysicalDevice() - Failed to find GPUs with Vulkan support!");

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

		for (auto& device : devices)
		{
			VkPhysicalDeviceProperties properties{};
			vkGetPhysicalDeviceProperties(device, &properties);
		
			if (IsDeviceSuitable(device))
			{
				m_PhysicalDevice = device;
				m_PhysicalDeviceName = properties.deviceName;
				EN_SUCCESS("Picked " + m_PhysicalDeviceName + " as the physical device!");
				break;
			}
			else
				EN_LOG(std::string(properties.deviceName) + " is not suitable to use as the physical device.");
		}

		if (m_PhysicalDevice == VK_NULL_HANDLE)
			EN_ERROR("Context.cpp::Context::VKPickPhysicalDevice() - Failed to find a suitable GPU!");
	}
	void Context::FindQueueFamilies(VkPhysicalDevice& device)
	{
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0U; const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				m_QueueFamilies.graphics = i;

			if ((queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) && !(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT))
				m_QueueFamilies.transfer = i;

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_WindowSurface, &presentSupport);

			if (presentSupport)
				m_QueueFamilies.present = i;

			if (m_QueueFamilies.IsComplete())
				break;

			i++;
		}

		if (!m_QueueFamilies.transfer.has_value() && m_QueueFamilies.graphics.has_value())
			m_QueueFamilies.transfer = m_QueueFamilies.graphics;
	}
	void Context::CreateLogicalDevice()
	{
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = { 
			m_QueueFamilies.graphics.value(), 
			m_QueueFamilies.transfer.value(), 
			m_QueueFamilies.present.value() 
		};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			const VkDeviceQueueCreateInfo queueCreateInfo{
				.sType			  = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
				.queueFamilyIndex = queueFamily,
				.queueCount		  = 1U,
				.pQueuePriorities = &queuePriority,
			};

			queueCreateInfos.emplace_back(queueCreateInfo);
		}

		VkDeviceCreateInfo createInfo{
			.sType					 = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
			.pNext					 = &deviceFeaturesVK1_1,
			.queueCreateInfoCount	 = static_cast<uint32_t>(queueCreateInfos.size()),
			.pQueueCreateInfos		 = queueCreateInfos.data(),
			.enabledLayerCount		 = 0U,
			.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size()),
			.ppEnabledExtensionNames = deviceExtensions.data(),
			.pEnabledFeatures		 = &deviceFeatures,
		};

		if constexpr (enableValidationLayers)
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
			
		if (vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_LogicalDevice) != VK_SUCCESS)
			EN_ERROR("Context.cpp::Context::VKCreateLogicalDevice() - Failed to create logical device!");

		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilies.graphics.value(), 0U, &m_GraphicsQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilies.transfer.value(), 0U, &m_TransferQueue);
		vkGetDeviceQueue(m_LogicalDevice, m_QueueFamilies.present.value(), 0U, &m_PresentQueue);
	}

	void Context::InitVMA()
	{
		VmaAllocatorCreateInfo allocatorInfo {
			.physicalDevice = m_PhysicalDevice,
			.device			= m_LogicalDevice,
			.instance		= m_Instance,
		};

		vmaCreateAllocator(&allocatorInfo, &m_Allocator);
	}

	void Context::CreateCommandPool()
	{
		const VkCommandPoolCreateInfo commandPoolCreateInfo {
			.sType			  = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags			  = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = m_QueueFamilies.graphics.value()
		};

		if (vkCreateCommandPool(m_LogicalDevice, &commandPoolCreateInfo, nullptr, &m_GraphicsCommandPool) != VK_SUCCESS)
			EN_ERROR("Context::VKCreateCommandPool() - Failed to create a graphics command pool!");

		const VkCommandPoolCreateInfo transferCommandPoolCreateInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
			.queueFamilyIndex = m_QueueFamilies.transfer.value(),
		};

		if (vkCreateCommandPool(m_LogicalDevice, &transferCommandPoolCreateInfo, nullptr, &m_TransferCommandPool) != VK_SUCCESS)
			EN_ERROR("Context::VKCreateCommandPool() - Failed to create a transfer command pool!");
	}
	void Context::CreateDescriptorAllocator()
	{
		m_DescriptorAllocator = MakeScope<DescriptorAllocator>(m_LogicalDevice);
	}

	bool Context::AreValidationLayerSupported()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers)
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
				return false;
		}

		return true;
	}
	std::vector<const char*> Context::GetRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0U;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if constexpr (enableValidationLayers)
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

		return extensions;
	}

	VkResult Context::CreateDebugUtilsMessengerEXT(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo)
	{
		PFN_vkCreateDebugUtilsMessengerEXT func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkCreateDebugUtilsMessengerEXT");

		if (func != nullptr)
			return func(m_Instance, pCreateInfo, nullptr, &m_DebugMessenger);

		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;

	}
	VkDebugUtilsMessengerCreateInfoEXT Context::CreateDebugMessengerCreateInfo()
	{
		return VkDebugUtilsMessengerCreateInfoEXT {
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = Context::DebugCallback,
		};
	}
	void Context::DestroyDebugUtilsMessengerEXT()
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_Instance, "vkDestroyDebugUtilsMessengerEXT");

		if (func != nullptr)
			func(m_Instance, m_DebugMessenger, nullptr);
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Context::DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			EN_LOG(std::string("[Diagnostic]: ") + pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			EN_LOG(std::string("[Info]: ") + pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			EN_WARN(std::string("[Warning]: ") + pCallbackData->pMessage);
			break;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			cl::Log(std::string("[Error]: ") + pCallbackData->pMessage + "\n", cl::Level::Error);
			break;
		}

		return VK_FALSE;
	}

	bool Context::IsDeviceSuitable(VkPhysicalDevice& device)
	{
		FindQueueFamilies(device);

		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapchainSupportDetails swapChainSupport;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_WindowSurface, &swapChainSupport.capabilities);

			uint32_t formatCount;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_WindowSurface, &formatCount, nullptr);

			if (formatCount != 0)
			{
				swapChainSupport.formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_WindowSurface, &formatCount, swapChainSupport.formats.data());
			}

			uint32_t presentModeCount;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_WindowSurface, &presentModeCount, nullptr);

			if (presentModeCount != 0)
			{
				swapChainSupport.presentModes.resize(presentModeCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_WindowSurface, &presentModeCount, swapChainSupport.presentModes.data());
			}

			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		bool featuresSupported = CheckDeviceFeaturesSupport(device);

		return m_QueueFamilies.IsComplete() && extensionsSupported && swapChainAdequate && featuresSupported;
	}
	bool Context::CheckDeviceExtensionSupport(VkPhysicalDevice& device)
	{
		uint32_t extensionCount{};
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}
	bool Context::CheckDeviceFeaturesSupport(VkPhysicalDevice& device)
	{
		VkPhysicalDeviceVulkan13Features supportedFeaturesVK1_3{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		};
		VkPhysicalDeviceVulkan12Features supportedFeaturesVK1_2{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		};
		VkPhysicalDeviceVulkan11Features supportedFeaturesVK1_1{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
		};
		VkPhysicalDeviceFeatures2 supportedFeatures{
			.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		};

		supportedFeatures.pNext = (void*)&supportedFeaturesVK1_1;
		vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);
		supportedFeatures.pNext = (void*)&supportedFeaturesVK1_2;
		vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);
		supportedFeatures.pNext = (void*)&supportedFeaturesVK1_3;
		vkGetPhysicalDeviceFeatures2(device, &supportedFeatures);

		return supportedFeatures.features.samplerAnisotropy && supportedFeaturesVK1_3.dynamicRendering && supportedFeaturesVK1_2.descriptorBindingUpdateUnusedWhilePending;
	}
}