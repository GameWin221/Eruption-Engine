#include <Core/EnPch.hpp>
#include "CameraMatricesBuffer.hpp"

#include <Renderer/Context.hpp>

namespace en
{
	VkDescriptorSetLayout g_CameraDescriptorSetLayout;
	VkDescriptorPool	  g_CameraDescriptorPool;

	void CreateCameraDescriptorPool();

	CameraMatricesBuffer::CameraMatricesBuffer()
	{
		for(auto& buffer : m_Buffers)
			buffer = std::make_unique<MemoryBuffer>(sizeof(CameraMatricesBufferObject), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		if (g_CameraDescriptorPool == VK_NULL_HANDLE)
			CreateCameraDescriptorPool();

		CreateDescriptorSets();
	}
	CameraMatricesBuffer::~CameraMatricesBuffer()
	{
		UseContext();

		vkFreeDescriptorSets(ctx.m_LogicalDevice, g_CameraDescriptorPool, FRAMES_IN_FLIGHT, m_DescriptorSets.data());
		vkDestroyDescriptorSetLayout(ctx.m_LogicalDevice, g_CameraDescriptorSetLayout, nullptr);
		vkDestroyDescriptorPool(ctx.m_LogicalDevice, g_CameraDescriptorPool, nullptr);

		g_CameraDescriptorPool = VK_NULL_HANDLE;
	}

	void CameraMatricesBuffer::UpdateMatrices(Camera* camera, uint32_t& frameIndex)
	{
		m_Matrices.proj = camera->GetProjMatrix();
		m_Matrices.view = camera->GetViewMatrix();

		m_Buffers[frameIndex]->MapMemory(&m_Matrices, m_Buffers[frameIndex]->GetSize());
	}

	void CameraMatricesBuffer::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout, uint32_t& frameIndex)
	{
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0U, 1U, &m_DescriptorSets[frameIndex], 0U, nullptr);
	}

	VkDescriptorSetLayout& CameraMatricesBuffer::GetLayout()
	{
		if (g_CameraDescriptorPool == VK_NULL_HANDLE)
			CreateCameraDescriptorPool();

		return g_CameraDescriptorSetLayout;
	}

	void CameraMatricesBuffer::CreateDescriptorSets()
	{
		UseContext();

		for (int i = 0; i < FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = g_CameraDescriptorPool;
			allocInfo.descriptorSetCount = 1U;
			allocInfo.pSetLayouts = &g_CameraDescriptorSetLayout;

			if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSets[i]) != VK_SUCCESS)
				EN_ERROR("CameraMatricesBuffer::CreateDescriptorSet() - Failed to allocate descriptor sets!");

			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = m_Buffers[i]->GetHandle();
			bufferInfo.offset = 0U;
			bufferInfo.range = sizeof(CameraMatricesBufferObject);

			VkWriteDescriptorSet descriptorWrite{};
			descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrite.dstSet = m_DescriptorSets[i];
			descriptorWrite.dstBinding = 0U;
			descriptorWrite.dstArrayElement = 0U;
			descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrite.descriptorCount = 1U;
			descriptorWrite.pBufferInfo = &bufferInfo;

			vkUpdateDescriptorSets(ctx.m_LogicalDevice, 1U, &descriptorWrite, 0U, nullptr);
		}
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
		poolSize.descriptorCount = FRAMES_IN_FLIGHT;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1U;
		poolInfo.pPoolSizes    = &poolSize;
		poolInfo.maxSets	   = FRAMES_IN_FLIGHT;
		poolInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_CameraDescriptorPool) != VK_SUCCESS)
			EN_ERROR("CameraMatricesBuffer::CreateCameraDescriptorPool() - Failed to create descriptor pool!");
	}
}