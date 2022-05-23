#include <Core/EnPch.hpp>
#include "Material.hpp"

namespace en
{
	Material* g_DefaultMaterial;
	
	VkDescriptorSetLayout g_MatDescriptorSetLayout;
	VkDescriptorPool g_MatDescriptorPool;

	void CreateMatDescriptorPool();

	Material::Material(std::string name, glm::vec3 color, float metalness, float normalStrength, Texture* albedoTexture, Texture* roughnessTexture, Texture* normalTexture)
		: m_Name(name), m_Color(color), m_Metalness(metalness), m_NormalStrength(normalStrength), m_Albedo(albedoTexture), m_Roughness(roughnessTexture), m_Normal(normalTexture)
	{
		if(g_MatDescriptorPool == VK_NULL_HANDLE)
			CreateMatDescriptorPool();

		m_Buffer = std::make_unique<MemoryBuffer>(sizeof(m_MatBuffer), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		CreateDescriptorSet();

		m_MatBuffer.color = m_Color;
		m_MatBuffer.metalness = m_Metalness;
		m_MatBuffer.normalStrength = m_NormalStrength;
		m_Buffer->MapMemory(&m_MatBuffer, m_Buffer->GetSize());
	}
	Material::~Material()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, g_MatDescriptorPool, 1, &m_DescriptorSet);
	}
	Material* Material::GetDefaultMaterial()
	{
		if (!g_DefaultMaterial)
			g_DefaultMaterial = new Material("No Material", glm::vec3(1.0f), 0.0f, 1.0f, Texture::GetWhiteSRGBTexture(), Texture::GetGreyNonSRGBTexture(), Texture::GetNormalTexture());

		return g_DefaultMaterial;
	}

	void Material::SetAlbedoTexture(Texture* texture)
	{
		m_Albedo = texture;
		m_UpdateQueued = true;
	}
	void Material::SetRoughnessTexture(Texture* texture)
	{
		m_Roughness = texture;
		m_UpdateQueued = true;
	}
	void Material::SetNormalTexture(Texture* texture)
	{
		m_Normal = texture;
		m_UpdateQueued = true;
	}

	void Material::Bind(VkCommandBuffer& cmd, VkPipelineLayout& layout)
	{
		const bool materialChanged = (m_MatBuffer.color != m_Color || m_MatBuffer.metalness != m_Metalness || m_MatBuffer.normalStrength != m_NormalStrength);

		if (materialChanged)
		{
			m_MatBuffer.color = m_Color;
			m_MatBuffer.metalness = m_Metalness;
			m_MatBuffer.normalStrength = m_NormalStrength;
			m_Buffer->MapMemory( &m_MatBuffer, m_Buffer->GetSize());
		}
		
		vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 1U, 1U, &m_DescriptorSet, 0U, nullptr);
	}
	void Material::UpdateDescriptorSet()
	{
		if (m_UpdateQueued)
		{
			UseContext();

			VkDescriptorBufferInfo matInfo{};
			matInfo.buffer = m_Buffer->GetHandle();
			matInfo.offset = 0;
			matInfo.range = sizeof(MatBuffer);

			VkDescriptorImageInfo albedoImageInfo{};
			albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			albedoImageInfo.imageView = m_Albedo->m_ImageView;
			albedoImageInfo.sampler = m_Albedo->m_ImageSampler;

			VkDescriptorImageInfo specularImageInfo{};
			specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			specularImageInfo.imageView = m_Roughness->m_ImageView;
			specularImageInfo.sampler = m_Roughness->m_ImageSampler;

			VkDescriptorImageInfo normalImageInfo{};
			normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			normalImageInfo.imageView = m_Normal->m_ImageView;
			normalImageInfo.sampler = m_Normal->m_ImageSampler;

			std::array<VkWriteDescriptorSet, 4> descriptorWrites{};
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

			descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[3].dstSet = m_DescriptorSet;
			descriptorWrites[3].dstBinding = 4;
			descriptorWrites[3].dstArrayElement = 0;
			descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[3].descriptorCount = 1;
			descriptorWrites[3].pImageInfo = &normalImageInfo;

			vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

			m_UpdateQueued = false;
		}
	}

	VkDescriptorSetLayout& Material::GetLayout()
	{
		if (g_MatDescriptorPool == VK_NULL_HANDLE)
			CreateMatDescriptorPool();

		return g_MatDescriptorSetLayout;
	}

	void CreateMatDescriptorPool()
	{
		UseContext();

		VkDescriptorSetLayoutBinding matBufferLayoutBinding{};
		matBufferLayoutBinding.binding = 1;
		matBufferLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		matBufferLayoutBinding.descriptorCount = 1;
		matBufferLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

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

		VkDescriptorSetLayoutBinding normalSamplerLayoutBinding{};
		normalSamplerLayoutBinding.binding = 4;
		normalSamplerLayoutBinding.descriptorCount = 1;
		normalSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalSamplerLayoutBinding.pImmutableSamplers = nullptr;
		normalSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings =
		{
			matBufferLayoutBinding,
			albedoSamplerLayoutBinding,
			specularSamplerLayoutBinding,
			normalSamplerLayoutBinding
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_MatDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("Material.cpp::CreateMatDescriptorPool() - Failed to create descriptor set layout!");

		std::array<VkDescriptorPoolSize, 4> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = 1U;
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = 1U;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = 1U;
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = 1U;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes	   = poolSizes.data();
		poolInfo.maxSets	   = static_cast<uint32_t>(MAX_MATERIALS);
		poolInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_MatDescriptorPool) != VK_SUCCESS)
			EN_ERROR("Material.cpp::CreateMatDescriptorPool() - Failed to create descriptor pool!");
	}
	void Material::CreateDescriptorSet()
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = g_MatDescriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts		 = &g_MatDescriptorSetLayout;

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		m_UpdateQueued = true;
		UpdateDescriptorSet();
	}
}