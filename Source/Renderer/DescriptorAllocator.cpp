#include "DescriptorAllocator.hpp"

#include <Core/Log.hpp>

namespace en 
{
	DescriptorAllocator* g_DescriptorAllocator = nullptr;

	DescriptorAllocator::DescriptorAllocator(VkDevice logicalDevice) : m_LogicalDevice(logicalDevice)
	{
		g_DescriptorAllocator = this;

		constexpr uint32_t poolSize = 1000U;

		constexpr std::array<VkDescriptorPoolSize, 11> poolSizes{
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLER,				  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,		  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,		  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,	  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,	  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,		  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,		  poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, poolSize },
			VkDescriptorPoolSize { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,		  poolSize }
		};

		constexpr uint32_t poolCount = static_cast<uint32_t>(poolSizes.size());

		VkDescriptorPoolCreateInfo poolInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets = poolSize * poolCount,
			.poolSizeCount = poolCount,
			.pPoolSizes = poolSizes.data()
		};

		if (vkCreateDescriptorPool(m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			EN_ERROR("DescriptorAllocator::DescriptorAllocator() - Failed to create ImGui's descriptor pool!");
	}

	DescriptorAllocator::~DescriptorAllocator()
	{
		g_DescriptorAllocator = nullptr;

		for (const auto& [info, layout] : m_LayoutMap)
			vkDestroyDescriptorSetLayout(m_LogicalDevice, layout, nullptr);

		vkDestroyDescriptorPool(m_LogicalDevice, m_DescriptorPool, nullptr);
	}

	VkDescriptorSetLayout DescriptorAllocator::MakeLayout(const DescriptorInfo& descriptorInfo)
	{
		if (m_LayoutMap.contains(descriptorInfo))
		{
			return m_LayoutMap.at(descriptorInfo);
		}
		else
		{
			VkDescriptorSetLayout newLayout = VK_NULL_HANDLE;

			std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorInfo.imageInfos.size() + descriptorInfo.bufferInfos.size());

			for (const auto& image : descriptorInfo.imageInfos)
			{
				bindings[image.index] = VkDescriptorSetLayoutBinding{
					.binding			= image.index,
					.descriptorType		= image.type,
					.descriptorCount	= image.count,
					.stageFlags			= image.stage,
					.pImmutableSamplers = nullptr
				};
			}

			for (const auto& buffer : descriptorInfo.bufferInfos)
			{
				bindings[buffer.index] = VkDescriptorSetLayoutBinding{
					.binding			= buffer.index,
					.descriptorType		= buffer.type,
					.descriptorCount	= 1U,
					.stageFlags			= buffer.stage,
					.pImmutableSamplers = nullptr
				};
			}

			std::vector<VkDescriptorBindingFlags> flags{};
			flags.resize(bindings.size(), descriptorInfo.bindingFlags);

			VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo {
				.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
				.bindingCount  = static_cast<uint32_t>(flags.size()),
				.pBindingFlags = flags.data(),
			};

			VkDescriptorSetLayoutCreateInfo layoutInfo {
				.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.pNext		  = &flagsCreateInfo,
				.flags		  = descriptorInfo.layoutFlags,
				.bindingCount = static_cast<uint32_t>(bindings.size()),
				.pBindings	  = bindings.data(),
			};

			if (vkCreateDescriptorSetLayout(m_LogicalDevice, &layoutInfo, nullptr, &newLayout) != VK_SUCCESS)
				EN_ERROR("DescriptorAllocator::MakeLayout() - Failed to create descriptor set layout!");

			m_LayoutMap[descriptorInfo] = newLayout;

			return newLayout;
		}
	}

	VkDescriptorSet DescriptorAllocator::MakeSet(const DescriptorInfo& descriptorInfo)
	{
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

		VkDescriptorSetLayout layout = MakeLayout(descriptorInfo);

		VkDescriptorSetAllocateInfo allocInfo{
			.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool		= m_DescriptorPool,
			.descriptorSetCount = 1U,
			.pSetLayouts		= &layout
		};

		if (vkAllocateDescriptorSets(m_LogicalDevice, &allocInfo, &descriptorSet) != VK_SUCCESS)
			EN_ERROR("DescriptorAllocator::MakeSet() - Failed to allocate a descriptor set!");

		return descriptorSet;
	}

	DescriptorAllocator& DescriptorAllocator::Get()
	{
		if (g_DescriptorAllocator)
			return *g_DescriptorAllocator;
		else
			EN_ERROR("DescriptorAllocator::Get() - g_DescriptorAllocator was a nullptr!");
	}
}

