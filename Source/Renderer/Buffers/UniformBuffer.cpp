#include <Core/EnPch.hpp>
#include "UniformBuffer.hpp"

namespace en
{
	VkDescriptorSetLayout g_UniformDescriptorSetLayout;
	VkDescriptorPool g_UniformDescriptorPool;
	VkDeviceSize g_UniformBufferSize;

	void CreateUniformDescriptorPool();

	UniformBuffer::UniformBuffer()
	{
		g_UniformBufferSize = sizeof(UniformBufferObject);

		en::Helpers::CreateBuffer(g_UniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);
	
		if(g_UniformDescriptorPool == VK_NULL_HANDLE)
			CreateUniformDescriptorPool();

		CreateDescriptorSet();
	}
	UniformBuffer::~UniformBuffer()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, g_UniformDescriptorPool, 1, &m_DescriptorSet);

		en::Helpers::DestroyBuffer(m_Buffer, m_BufferMemory);
	}

	void UniformBuffer::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout)
	{
		m_UBO.model = m_Model;
		m_UBO.view = m_View;
		m_UBO.proj = m_Proj;

		en::Helpers::MapBuffer(m_BufferMemory, &m_UBO, g_UniformBufferSize);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, 1, &m_DescriptorSet, 0, nullptr);
	}
	VkDescriptorSetLayout& UniformBuffer::GetUniformDescriptorLayout()
	{
		if (g_UniformDescriptorPool == VK_NULL_HANDLE)
			CreateUniformDescriptorPool();

		return g_UniformDescriptorSetLayout;
	}
	void CreateUniformDescriptorPool()
	{
		UseContext();

		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1U;
		layoutInfo.pBindings = &uboLayoutBinding;

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_UniformDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorPool() - Failed to create descriptor set layout!");

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = static_cast<uint32_t>(1);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1U;
		poolInfo.pPoolSizes = &poolSize;
		poolInfo.maxSets = static_cast<uint32_t>(MAX_MESHES);
		poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_UniformDescriptorPool) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void UniformBuffer::CreateDescriptorSet()
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = g_UniformDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
		allocInfo.pSetLayouts = &g_UniformDescriptorSetLayout;

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_Buffer;
		bufferInfo.offset = 0;
		bufferInfo.range  = sizeof(UniformBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet = m_DescriptorSet;
		descriptorWrite.dstBinding = 0;
		descriptorWrite.dstArrayElement = 0;
		descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1;
		descriptorWrite.pBufferInfo = &bufferInfo;

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, 1U, &descriptorWrite, 0, nullptr);
	}
}