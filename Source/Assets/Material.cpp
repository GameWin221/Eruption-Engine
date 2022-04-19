#include <Core/EnPch.hpp>
#include "Material.hpp"

namespace en
{
	Material* g_DefaultMaterial;
	
	VkDescriptorSetLayout g_MatDescriptorSetLayout;
	VkDescriptorPool g_MatDescriptorPool;
	VkDeviceSize g_MatBufferSize;

	void CreateMatDescriptorPool();

	Material::Material(glm::vec3 color, float shininess, Texture* albedoTexture, Texture* specularTexture) : m_Color(color), m_Shininess(shininess)
	{
		if (!albedoTexture) m_Albedo = Texture::GetWhiteTexture();
		else m_Albedo = albedoTexture;

		if (!specularTexture) m_Specular = Texture::GetWhiteTexture();
		else m_Specular = specularTexture;

		if(g_MatDescriptorPool == VK_NULL_HANDLE)
			CreateMatDescriptorPool();

		CreateMatBuffer();
		CreateDescriptorSet();
	}
	Material::~Material()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, g_MatDescriptorPool, 1, &m_DescriptorSet);

		Helpers::DestroyBuffer(m_Buffer, m_BufferMemory);
	}
	Material* Material::GetDefaultMaterial()
	{
		if (!g_DefaultMaterial)
			g_DefaultMaterial = new Material(glm::vec3(1.0f), 32.0f, nullptr, nullptr);

		return g_DefaultMaterial;
	}
	void Material::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout)
	{
		m_MatBuffer.Color = m_Color;
		m_MatBuffer.Shininess = m_Shininess;

		en::Helpers::MapBuffer(m_BufferMemory, &m_MatBuffer, g_MatBufferSize);
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1, 1, &m_DescriptorSet, 0, nullptr);
	}
	void Material::Update()
	{
		UseContext();

		VkDescriptorBufferInfo matInfo{};
		matInfo.buffer = m_Buffer;
		matInfo.offset = 0;
		matInfo.range = sizeof(MatBuffer);

		VkDescriptorImageInfo albedoImageInfo{};
		albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoImageInfo.imageView = m_Albedo->m_ImageView;
		albedoImageInfo.sampler = m_Albedo->m_ImageSampler;

		VkDescriptorImageInfo specularImageInfo{};
		specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		specularImageInfo.imageView = m_Specular->m_ImageView;
		specularImageInfo.sampler = m_Specular->m_ImageSampler;

		std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSet;
		descriptorWrites[0].dstBinding = 1;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &matInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_DescriptorSet;
		descriptorWrites[1].dstBinding = 2;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &albedoImageInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = m_DescriptorSet;
		descriptorWrites[2].dstBinding = 3;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &specularImageInfo;

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
	VkDescriptorSetLayout& Material::GetMatDescriptorLayout()
	{
		if (g_MatDescriptorPool == VK_NULL_HANDLE)
			CreateMatDescriptorPool();

		return g_MatDescriptorSetLayout;
	}

	void CreateMatDescriptorPool()
	{
		UseContext();

		VkDescriptorSetLayoutBinding matLayoutBinding{};
		matLayoutBinding.binding = 1;
		matLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		matLayoutBinding.descriptorCount = 1;
		matLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding albedoSamplerLayoutBinding{};
		albedoSamplerLayoutBinding.binding = 2;
		albedoSamplerLayoutBinding.descriptorCount = 1;
		albedoSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoSamplerLayoutBinding.pImmutableSamplers = nullptr;
		albedoSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding specularSamplerLayoutBinding{};
		specularSamplerLayoutBinding.binding = 3;
		specularSamplerLayoutBinding.descriptorCount = 1;
		specularSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		specularSamplerLayoutBinding.pImmutableSamplers = nullptr;
		specularSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 3> bindings =
		{
			matLayoutBinding,
			albedoSamplerLayoutBinding,
			specularSamplerLayoutBinding
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_MatDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to create descriptor set layout!");

		std::array<VkDescriptorPoolSize, 3> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(1);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(1);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(1);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes	   = poolSizes.data();
		poolInfo.maxSets	   = static_cast<uint32_t>(MAX_MATERIALS);
		poolInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_MatDescriptorPool) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to create descriptor pool!");
	}
	void Material::CreateDescriptorSet()
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = g_MatDescriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(1);
		allocInfo.pSetLayouts		 = &g_MatDescriptorSetLayout;

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		Update();
	}
	void Material::CreateMatBuffer()
	{
		g_MatBufferSize = sizeof(m_MatBuffer);
		en::Helpers::CreateBuffer(g_MatBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_Buffer, m_BufferMemory);
	}
}