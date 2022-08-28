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

	void CameraMatricesBuffer::MapBuffer(uint32_t frameIndex)
	{
		m_Buffers[frameIndex]->MapMemory(&m_Matrices[frameIndex], m_Buffers[frameIndex]->m_BufferSize);
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
			const VkDescriptorSetAllocateInfo allocInfo{
				.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
				.descriptorPool		= g_CameraDescriptorPool,
				.descriptorSetCount = 1U,
				.pSetLayouts		= &g_CameraDescriptorSetLayout
			};

			if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSets[i]) != VK_SUCCESS)
				EN_ERROR("CameraMatricesBuffer::CreateDescriptorSet() - Failed to allocate descriptor sets!");

			const VkDescriptorBufferInfo bufferInfo{
				.buffer = m_Buffers[i]->GetHandle(),
				.offset = 0U,
				.range  = sizeof(CameraMatricesBufferObject)
			};

			const VkWriteDescriptorSet descriptorWrite{
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSets[i],
				.dstBinding		 = 0U,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo	 = &bufferInfo
			};

			vkUpdateDescriptorSets(ctx.m_LogicalDevice, 1U, &descriptorWrite, 0U, nullptr);
		}
	}
	void CreateCameraDescriptorPool()
	{
		UseContext();

		const VkDescriptorSetLayoutBinding layoutBinding{
			.binding		 = 0U,
			.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1U,
			.stageFlags		 = VK_SHADER_STAGE_VERTEX_BIT
		};

		const VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1U,
			.pBindings	  = &layoutBinding
		};

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_CameraDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("CameraMatricesBuffer::CreateCameraDescriptorPool() - Failed to create descriptor set layout!");

		const VkDescriptorPoolSize poolSize{
			.type			 = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = FRAMES_IN_FLIGHT
		};

		const VkDescriptorPoolCreateInfo poolInfo{
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
			.maxSets	   = FRAMES_IN_FLIGHT,
			.poolSizeCount = 1U,
			.pPoolSizes	   = &poolSize
		};

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_CameraDescriptorPool) != VK_SUCCESS)
			EN_ERROR("CameraMatricesBuffer::CreateCameraDescriptorPool() - Failed to create descriptor pool!");
	}
}