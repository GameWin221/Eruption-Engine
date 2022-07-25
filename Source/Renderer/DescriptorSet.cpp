#include "Core/EnPch.hpp"
#include "DescriptorSet.hpp"

#include <Common/Helpers.hpp>
namespace en
{
	DescriptorSet::DescriptorSet(const std::vector<ImageInfo>& imageInfos, const BufferInfo& bufferInfo)
	{
		CreateDescriptorPool(imageInfos, bufferInfo);
		CreateDescriptorSet();
		Update(imageInfos, bufferInfo);
	}
	DescriptorSet::~DescriptorSet()
	{
		UseContext();

		vkDestroyDescriptorSetLayout(ctx.m_LogicalDevice, m_DescriptorLayout, nullptr);
		vkDestroyDescriptorPool(ctx.m_LogicalDevice, m_DescriptorPool, nullptr);
	}
	
	void DescriptorSet::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, const uint32_t& index, const VkPipelineBindPoint& bindPoint)
	{
		vkCmdBindDescriptorSets(cmd, bindPoint, layout, index, 1U, &m_DescriptorSet, 0U, nullptr);
	}
	void DescriptorSet::CreateDescriptorPool(const std::vector<ImageInfo>& imageInfos, const BufferInfo& bufferInfo)
	{
		UseContext();

		std::vector<VkDescriptorSetLayoutBinding> bindings;

		for (const auto& image : imageInfos)
		{
			const VkDescriptorSetLayoutBinding binding{
				.binding			= image.index,
				.descriptorType	    = image.type,
				.descriptorCount	= image.count,
				.stageFlags			= static_cast<VkShaderStageFlags>(image.stage),
				.pImmutableSamplers = nullptr
			};

			bindings.emplace_back(binding);
		}

		if (bufferInfo.buffer != VK_NULL_HANDLE)
		{
			const VkDescriptorSetLayoutBinding binding{
				.binding			= bufferInfo.index,
				.descriptorType		= bufferInfo.type,
				.descriptorCount	= 1U,
				.stageFlags			= static_cast<VkShaderStageFlags>(bufferInfo.stage),
				.pImmutableSamplers = nullptr
			};

			bindings.emplace_back(binding);
		}

		const VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings	  = bindings.data()
		};

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorLayout) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorPool() - Failed to create descriptor set layout!");

		std::vector<VkDescriptorPoolSize> poolSizes;
		for (const auto& binding : bindings)
		{
			const VkDescriptorPoolSize size {
				.type			 = binding.descriptorType,
				.descriptorCount = 1U
			};

			poolSizes.emplace_back(size);
		}

		const VkDescriptorPoolCreateInfo poolInfo{
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets	   = 1U,
			.poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes	   = poolSizes.data(),
		};

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void DescriptorSet::CreateDescriptorSet()
	{
		UseContext();

		const VkDescriptorSetAllocateInfo allocInfo{
			.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool		= m_DescriptorPool,
			.descriptorSetCount = 1U,
			.pSetLayouts		= &m_DescriptorLayout
		};

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorSet() - Failed to allocate descriptor sets!");
	}
	void DescriptorSet::Update(const std::vector<ImageInfo>& imageInfos, const BufferInfo& bufferInfo)
	{
		UseContext();

		std::vector<VkDescriptorImageInfo> descriptorImageInfos;
		for (const auto& image : imageInfos)
		{
			const VkDescriptorImageInfo info{
				.sampler	 = image.imageSampler,
				.imageView   = image.imageView,
				.imageLayout = image.imageLayout
			};

			descriptorImageInfos.emplace_back(info);
		}

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		for (int i = 0; const auto & image : imageInfos)
		{
			const VkWriteDescriptorSet descriptorWrite{
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = image.index,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType	 = image.type,
				.pImageInfo		 = &descriptorImageInfos[i++]
			};

			descriptorWrites.emplace_back(descriptorWrite);
		}

		const VkDescriptorBufferInfo info{
			.buffer = bufferInfo.buffer,
			.offset = 0U,
			.range  = bufferInfo.size
		};

		if (bufferInfo.buffer != VK_NULL_HANDLE)
		{
			const VkWriteDescriptorSet descriptorWrite{
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = bufferInfo.index,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = bufferInfo.type,
				.pBufferInfo	 = &info
			};

			descriptorWrites.emplace_back(descriptorWrite);
		}

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0U, nullptr);
	}
}