#include "DescriptorSet.hpp"

#include <Common/Helpers.hpp>

namespace en
{
	DescriptorSet::DescriptorSet(const std::vector<ImageInfo>& imageInfos, const std::vector<BufferInfo>& bufferInfos)
	{
		CreateDescriptorPool(imageInfos, bufferInfos);
		CreateDescriptorSet();

		Update(imageInfos, bufferInfos);
	}
	DescriptorSet::~DescriptorSet()
	{
		UseContext();

		vkDestroyDescriptorSetLayout(ctx.m_LogicalDevice, m_DescriptorLayout, nullptr);
		vkDestroyDescriptorPool(ctx.m_LogicalDevice, m_DescriptorPool, nullptr);
	}
	
	void DescriptorSet::CreateDescriptorPool(const std::vector<ImageInfo>& imageInfos, const std::vector<BufferInfo>& bufferInfos)
	{
		UseContext();

		std::vector<VkDescriptorSetLayoutBinding> bindings(imageInfos.size() + bufferInfos.size());

		for (const auto& image : imageInfos)
		{
			bindings[image.index] = VkDescriptorSetLayoutBinding {
				.binding = image.index,
				.descriptorType = image.type,
				.descriptorCount = image.count,
				.stageFlags = image.stage,
				.pImmutableSamplers = nullptr
			};
		}

		for (const auto& buffer : bufferInfos)
		{
			bindings[buffer.index] = VkDescriptorSetLayoutBinding {
				.binding = buffer.index,
				.descriptorType = buffer.type,
				.descriptorCount = 1U,
				.stageFlags = buffer.stage,
				.pImmutableSamplers = nullptr
			};
		}

		const VkDescriptorSetLayoutCreateInfo layoutInfo {
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
	void DescriptorSet::Update(const std::vector<ImageInfo>& imageInfos, const std::vector<BufferInfo>& bufferInfos)
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

		std::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
		for (const auto& buffer : bufferInfos)
		{
			const VkDescriptorBufferInfo info{
				.buffer = buffer.buffer,
				.offset = 0U,
				.range  = buffer.size
			};

			descriptorBufferInfos.emplace_back(info);
		}

		std::vector<VkWriteDescriptorSet> descriptorWrites(imageInfos.size() + bufferInfos.size());
		for (uint32_t i = 0; const auto & image : imageInfos)
		{
			descriptorWrites[image.index] = VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSet,
				.dstBinding = image.index,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType = image.type,
				.pImageInfo = &descriptorImageInfos[i++]
			};
		}
		for (uint32_t i = 0; const auto & buffer : bufferInfos)
		{
			descriptorWrites[buffer.index] = VkWriteDescriptorSet {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = m_DescriptorSet,
				.dstBinding = buffer.index,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType = buffer.type,
				.pBufferInfo = &descriptorBufferInfos[i++]
			};
		}

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0U, nullptr);
	}
}