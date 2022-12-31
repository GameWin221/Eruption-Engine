#include <Core/EnPch.hpp>
#include "Material.hpp"

namespace en
{
	Material* g_DefaultMaterial;
	
	VkDescriptorSetLayout g_MatDescriptorSetLayout;
	VkDescriptorPool g_MatDescriptorPool;

	void CreateMatDescriptorPool();

	Material::Material(const std::string& name, const glm::vec3 color, const float metalnessVal, const float roughnessVal, const float normalStrength, Texture* albedoTexture, Texture* roughnessTexture, Texture* normalTexture, Texture* metalnessTexture)
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

		VkDescriptorImageInfo albedoImageInfo {
			.sampler	 = m_Albedo->m_ImageSampler,
			.imageView   = m_Albedo->m_Image->m_ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkDescriptorImageInfo specularImageInfo{
			.sampler	 = m_Roughness->m_ImageSampler,
			.imageView   = m_Roughness->m_Image->m_ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkDescriptorImageInfo normalImageInfo{
			.sampler	 = m_Normal->m_ImageSampler,
			.imageView	 = m_Normal->m_Image->m_ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkDescriptorImageInfo metalnessImageInfo{
			.sampler	 = m_Metalness->m_ImageSampler,
			.imageView   = m_Metalness->m_Image->m_ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		};

		std::array<VkWriteDescriptorSet, 4> descriptorWrites{
			VkWriteDescriptorSet {
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = 1U,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo		 = &albedoImageInfo
			},

			VkWriteDescriptorSet {
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = 2U,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo		 = &specularImageInfo
			},

			VkWriteDescriptorSet {
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = 3U,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo		 = &normalImageInfo
			},

			VkWriteDescriptorSet {
				.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet			 = m_DescriptorSet,
				.dstBinding		 = 4U,
				.dstArrayElement = 0U,
				.descriptorCount = 1U,
				.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo		 = &metalnessImageInfo
			},
		};

		UpdateBuffer();

		vkUpdateDescriptorSets(
			ctx.m_LogicalDevice, 
			static_cast<uint32_t>(descriptorWrites.size()),
			descriptorWrites.data(),
			0U,
			nullptr
		);

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

		VkDescriptorSetLayoutBinding albedoLayoutBinding {
			.binding			= 1U,
			.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount	= 1U,
			.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutBinding specularLayoutBinding {
			.binding			= 2U,
			.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount	= 1U,
			.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutBinding normalLayoutBinding {
			.binding			= 3U,
			.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount	= 1U,
			.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		VkDescriptorSetLayoutBinding metalnessLayoutBinding {
			.binding			= 4U,
			.descriptorType		= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount	= 1U,
			.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT,
			.pImmutableSamplers = nullptr,
		};

		std::array<VkDescriptorSetLayoutBinding, 4> bindings {
			albedoLayoutBinding,
			specularLayoutBinding,
			normalLayoutBinding,
			metalnessLayoutBinding
		};

		std::array<VkDescriptorBindingFlags, 4> flags { 
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT,
			VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT
		};

		VkDescriptorSetLayoutBindingFlagsCreateInfo flagsCreateInfo {
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
			.bindingCount  = static_cast<uint32_t>(bindings.size()),
			.pBindingFlags = flags.data(),
		};

		VkDescriptorSetLayoutCreateInfo layoutInfo {
			.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.pNext		  = &flagsCreateInfo,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings	  = bindings.data(),
		};


		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &g_MatDescriptorSetLayout) != VK_SUCCESS)
			EN_ERROR("Material.cpp::CreateMatDescriptorPool() - Failed to create descriptor set layout!");

		constexpr std::array<VkDescriptorPoolSize, 4> poolSizes {
			VkDescriptorPoolSize {
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = static_cast<uint32_t>(MAX_MATERIALS),
			},
			VkDescriptorPoolSize {
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = static_cast<uint32_t>(MAX_MATERIALS),
			},
			VkDescriptorPoolSize {
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = static_cast<uint32_t>(MAX_MATERIALS),
			},
			VkDescriptorPoolSize {
				.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.descriptorCount = static_cast<uint32_t>(MAX_MATERIALS),
			},
		};

		VkDescriptorPoolCreateInfo poolInfo {
			.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.flags		    = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT | VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
			.maxSets		= static_cast<uint32_t>(MAX_MATERIALS),
			.poolSizeCount  = static_cast<uint32_t>(poolSizes.size()),
			.pPoolSizes		= poolSizes.data(),
		};

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &g_MatDescriptorPool) != VK_SUCCESS)
			EN_ERROR("Material.cpp::CreateMatDescriptorPool() - Failed to create descriptor pool!");
	}
	void Material::CreateDescriptorSet()
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo {
			.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool		= g_MatDescriptorPool,
			.descriptorSetCount = 1U,
			.pSetLayouts		= &g_MatDescriptorSetLayout,
		};

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		m_UpdateQueued = true;
		Update();
	}
}