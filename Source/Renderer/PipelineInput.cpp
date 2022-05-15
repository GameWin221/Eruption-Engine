#include "Core/EnPch.hpp"
#include "PipelineInput.hpp"

#include <Common/Helpers.hpp>
namespace en
{
	PipelineInput::PipelineInput(std::vector<ImageInfo> imageInfos, BufferInfo bufferInfo)
	{
		CreateDescriptorPool(imageInfos, bufferInfo);
		CreateDescriptorSet(imageInfos, bufferInfo);
		UpdateDescriptorSet(imageInfos, bufferInfo);
	}
	PipelineInput::~PipelineInput()
	{
		UseContext();

		vkDestroyDescriptorSetLayout(ctx.m_LogicalDevice, m_DescriptorLayout, nullptr);
		vkDestroyDescriptorPool(ctx.m_LogicalDevice, m_DescriptorPool, nullptr);
	}
	
	void PipelineInput::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, uint32_t index)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, index, 1U, &m_DescriptorSet, 0U, nullptr);
	}
	void PipelineInput::CreateDescriptorPool(std::vector<ImageInfo>& imageInfos, BufferInfo& bufferInfo)
	{
		UseContext();

		std::vector<VkDescriptorSetLayoutBinding> bindings;

		for (const auto& image : imageInfos)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding			   = image.index;
			binding.descriptorCount	   = 1U;
			binding.descriptorType	   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			binding.pImmutableSamplers = nullptr;
			binding.stageFlags		   = VK_SHADER_STAGE_FRAGMENT_BIT;

			bindings.emplace_back(binding);
		}

		if (bufferInfo.buffer != VK_NULL_HANDLE)
		{
			VkDescriptorSetLayoutBinding binding{};
			binding.binding			= bufferInfo.index;
			binding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			binding.descriptorCount = 1U;
			binding.stageFlags		= VK_SHADER_STAGE_FRAGMENT_BIT;

			bindings.emplace_back(binding);
		}

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings	= bindings.data();

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorLayout) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorPool() - Failed to create descriptor set layout!");


		std::vector<VkDescriptorPoolSize> poolSizes;
		for (const auto& binding : bindings)
		{
			VkDescriptorPoolSize size{};
			size.type = binding.descriptorType;
			size.descriptorCount = 1U;

			poolSizes.emplace_back(size);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes	   = poolSizes.data();
		poolInfo.maxSets	   = 1U;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void PipelineInput::CreateDescriptorSet(std::vector<ImageInfo>& imageInfos, BufferInfo& bufferInfo)
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts		 = &m_DescriptorLayout;

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorSet() - Failed to allocate descriptor sets!");
		}
	void PipelineInput::UpdateDescriptorSet(std::vector<ImageInfo> imageInfos, BufferInfo bufferInfo)
	{
		UseContext();

		std::vector<VkDescriptorImageInfo> descriptorImageInfos;
		for (const auto& image : imageInfos)
		{
			VkDescriptorImageInfo info{};
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			info.imageView = image.imageView;
			info.sampler = image.imageSampler;
			descriptorImageInfos.emplace_back(info);
		}

		std::vector<VkWriteDescriptorSet> descriptorWrites;
		for (int i = 0; const auto & image : imageInfos)
		{
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSet;
			descriptorWrite.dstBinding = image.index;
			descriptorWrite.dstArrayElement = 0U;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrite.descriptorCount = 1U;
			descriptorWrite.pImageInfo = &descriptorImageInfos[i++];

			descriptorWrites.emplace_back(descriptorWrite);
		}

		VkDescriptorBufferInfo info{};
		info.buffer = bufferInfo.buffer;
		info.offset = 0U;
		info.range = bufferInfo.size;

		if (bufferInfo.buffer != VK_NULL_HANDLE)
		{
			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSet;
			descriptorWrite.dstBinding = bufferInfo.index;
			descriptorWrite.dstArrayElement = 0U;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1U;
			descriptorWrite.pBufferInfo = &info;

			descriptorWrites.emplace_back(descriptorWrite);
		}

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0U, nullptr);

	}
}