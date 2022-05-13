#include <Core/EnPch.hpp>
#include "CameraMatricesBuffer.hpp"

namespace en
{
	VkDescriptorSetLayout g_CameraDescriptorSetLayout;
	VkDescriptorPool	  g_CameraDescriptorPool;

	void CreateCameraDescriptorPool();

	CameraMatricesBuffer::CameraMatricesBuffer()
	{
		m_BufferSize = sizeof(CameraMatricesBufferObject);

		en::Helpers::CreateBuffer(m_BufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);

		if (g_CameraDescriptorPool == VK_NULL_HANDLE)
			CreateCameraDescriptorPool();

		CreateDescriptorSet();
	}
	CameraMatricesBuffer::~CameraMatricesBuffer()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, g_CameraDescriptorPool, 1U, &m_DescriptorSet);

		en::Helpers::DestroyBuffer(m_Buffer, m_BufferMemory);
	}

	void CameraMatricesBuffer::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout)
	{
		en::Helpers::MapBuffer(m_BufferMemory, &m_Matrices, m_BufferSize);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0U, 1U, &m_DescriptorSet, 0U, nullptr);
	}

	VkDescriptorSetLayout& CameraMatricesBuffer::GetLayout()
	{
		if (g_CameraDescriptorPool == VK_NULL_HANDLE)
			CreateCameraDescriptorPool();

		return g_CameraDescriptorSetLayout;
	}

	void CameraMatricesBuffer::CreateDescriptorSet()
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = g_CameraDescriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts		 = &g_CameraDescriptorSetLayout;

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("UniformBuffer::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		VkDescriptorBufferInfo bufferInfo{};
		bufferInfo.buffer = m_Buffer;
		bufferInfo.offset = 0U;
		bufferInfo.range  = sizeof(CameraMatricesBufferObject);

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet			= m_DescriptorSet;
		descriptorWrite.dstBinding		= 0U;
		descriptorWrite.dstArrayElement = 0U;
		descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrite.descriptorCount = 1U;
		descriptorWrite.pBufferInfo		= &bufferInfo;

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, 1U, &descriptorWrite, 0U, nullptr);
	}
	void CreateCameraDescriptorPool()
	{
		UseContext();

		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding		  = 0U;
		layoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		layoutBinding.descriptorCount = 1U;
		layoutBinding.stageFlags	  = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1U;
		layoutInfo.pBindings	= &layoutBinding;

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_CameraDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("CameraMatricesBuffer::CreateCameraDescriptorPool() - Failed to create descriptor set layout!");

		VkDescriptorPoolSize poolSize{};
		poolSize.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSize.descriptorCount = 1U;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1U;
		poolInfo.pPoolSizes    = &poolSize;
		poolInfo.maxSets	   = 1U;
		poolInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_CameraDescriptorPool) != VK_SUCCESS)
			EN_ERROR("CameraMatricesBuffer::CreateCameraDescriptorPool() - Failed to create descriptor pool!");
	}
}