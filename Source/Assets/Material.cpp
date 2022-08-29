#include <Core/EnPch.hpp>
#include "Material.hpp"

namespace en
{
	Material* g_DefaultMaterial;
	
	VkDescriptorSetLayout g_MatDescriptorSetLayout;
	VkDescriptorPool g_MatDescriptorPool;

	void CreateMatDescriptorPool();

	Material::Material(const std::string& name, const glm::vec3& color, const float& metalnessVal, const float& roughnessVal, const float& normalStrength, Texture* albedoTexture, Texture* roughnessTexture, Texture* normalTexture, Texture* metalnessTexture)
		: m_Name(name), m_Color(color), m_MetalnessVal(metalnessVal), m_RoughnessVal(roughnessVal), m_NormalStrength(normalStrength), m_Albedo(albedoTexture), m_Roughness(roughnessTexture), m_Metalness(metalnessTexture), m_Normal(normalTexture), Asset{ AssetType::Material }
	{
		if(g_MatDescriptorPool == VK_NULL_HANDLE)
			CreateMatDescriptorPool();

		CreateDescriptorSet();

		UpdateBuffer();
	}
	Material::~Material()
	{
		UseContext();
		vkFreeDescriptorSets(ctx.m_LogicalDevice, g_MatDescriptorPool, 1, &m_DescriptorSet);
	}
	Material* Material::GetDefaultMaterial()
	{
		if (!g_DefaultMaterial)
			g_DefaultMaterial = new Material("No Material", glm::vec3(1.0f), 0.0f, 0.75f, 0.0f, Texture::GetWhiteSRGBTexture(), Texture::GetWhiteNonSRGBTexture(), Texture::GetWhiteNonSRGBTexture(), Texture::GetWhiteNonSRGBTexture());

		return g_DefaultMaterial;
	}

	void Material::SetColor(glm::vec3 color)
	{
		m_Color = color;
		m_UpdateQueued = true;
	}
	void Material::SetNormalStrength(float normalStrength)
	{
		m_NormalStrength = normalStrength;
		m_UpdateQueued = true;
	}
	void Material::SetMetalness(float metalness)
	{
		m_MetalnessVal = metalness;
		m_UpdateQueued = true;
	}
	void Material::SetRoughness(float roughness)
	{
		m_RoughnessVal = roughness;
		m_UpdateQueued = true;
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
	void Material::SetMetalnessTexture(Texture* texture)
	{
		m_Metalness = texture;
		m_UpdateQueued = true;
	}
	void Material::SetNormalTexture(Texture* texture)
	{
		m_Normal = texture;
		m_UpdateQueued = true;
	}

	void Material::Update()
	{
		if (!m_UpdateQueued) return;
		
		UseContext();

		VkDescriptorImageInfo albedoImageInfo{};
		albedoImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		albedoImageInfo.imageView = m_Albedo->m_Image->m_ImageView;
		albedoImageInfo.sampler = m_Albedo->m_ImageSampler;

		VkDescriptorImageInfo specularImageInfo{};
		specularImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		specularImageInfo.imageView = m_Roughness->m_Image->m_ImageView;
		specularImageInfo.sampler = m_Roughness->m_ImageSampler;

		VkDescriptorImageInfo normalImageInfo{};
		normalImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		normalImageInfo.imageView = m_Normal->m_Image->m_ImageView;
		normalImageInfo.sampler = m_Normal->m_ImageSampler;

		VkDescriptorImageInfo metalnessImageInfo{};
		metalnessImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		metalnessImageInfo.imageView = m_Metalness->m_Image->m_ImageView;
		metalnessImageInfo.sampler = m_Metalness->m_ImageSampler;

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = m_DescriptorSet;
		descriptorWrites[0].dstBinding = 1;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pImageInfo = &albedoImageInfo;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = m_DescriptorSet;
		descriptorWrites[1].dstBinding = 2;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pImageInfo = &specularImageInfo;

		descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[2].dstSet = m_DescriptorSet;
		descriptorWrites[2].dstBinding = 3;
		descriptorWrites[2].dstArrayElement = 0;
		descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[2].descriptorCount = 1;
		descriptorWrites[2].pImageInfo = &normalImageInfo;

		descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[3].dstSet = m_DescriptorSet;
		descriptorWrites[3].dstBinding = 4;
		descriptorWrites[3].dstArrayElement = 0;
		descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[3].descriptorCount = 1;
		descriptorWrites[3].pImageInfo = &metalnessImageInfo;

		UpdateBuffer();

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);

		m_UpdateQueued = false;
	}
	void Material::UpdateBuffer()
	{
		m_MatBuffer.color		   = m_Color;
		m_MatBuffer.metalnessVal   = m_MetalnessVal;
		m_MatBuffer.roughnessVal   = m_RoughnessVal;
		m_MatBuffer.normalStrength = m_NormalStrength;

		if (m_Roughness != Texture::GetWhiteNonSRGBTexture())
			m_MatBuffer.roughnessVal = 1.0f;

		if (m_Metalness != Texture::GetWhiteNonSRGBTexture())
			m_MatBuffer.metalnessVal = 1.0f;

		if (m_Normal == Texture::GetWhiteNonSRGBTexture())
			m_MatBuffer.normalStrength = 0.0f;
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

		VkDescriptorSetLayoutBinding albedoLayoutBinding{};
		albedoLayoutBinding.binding = 1;
		albedoLayoutBinding.descriptorCount = 1;
		albedoLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		albedoLayoutBinding.pImmutableSamplers = nullptr;
		albedoLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding specularLayoutBinding{};
		specularLayoutBinding.binding = 2;
		specularLayoutBinding.descriptorCount = 1;
		specularLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		specularLayoutBinding.pImmutableSamplers = nullptr;
		specularLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding normalLayoutBinding{};
		normalLayoutBinding.binding = 3;
		normalLayoutBinding.descriptorCount = 1;
		normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normalLayoutBinding.pImmutableSamplers = nullptr;
		normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding metalnessLayoutBinding{};
		metalnessLayoutBinding.binding = 4;
		metalnessLayoutBinding.descriptorCount = 1;
		metalnessLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		metalnessLayoutBinding.pImmutableSamplers = nullptr;
		metalnessLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 4> bindings =
		{
			albedoLayoutBinding,
			specularLayoutBinding,
			normalLayoutBinding,
			metalnessLayoutBinding
		};

		std::array<VkDescriptorBindingFlags, 4> flags = 
		{ 
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT
		};

		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo{};
		flagsCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
		flagsCreateInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		flagsCreateInfo.pBindingFlags = flags.data();

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();
		layoutInfo.pNext = &flagsCreateInfo;


		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_MatDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("Material.cpp::CreateMatDescriptorPool() - Failed to create descriptor set layout!");

		std::array<VkDescriptorPoolSize, 4> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_MATERIALS);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_MATERIALS);
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_MATERIALS);
		poolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[3].descriptorCount = static_cast<uint32_t>(MAX_MATERIALS);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes	   = poolSizes.data();
		poolInfo.maxSets	   = static_cast<uint32_t>(MAX_MATERIALS);
		poolInfo.flags		   = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;

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
		Update();
	}
}