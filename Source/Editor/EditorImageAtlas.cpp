#include "Core/EnPch.hpp"
#include "EditorImageAtlas.hpp"

namespace en
{
	EditorImageAtlas::EditorImageAtlas(std::string atlasPath, uint32_t x, int32_t y)
	{
		m_Texture = std::make_unique<Texture>(atlasPath, "EditorImageAtlas", VK_FORMAT_R8G8B8A8_SRGB, false, false);

		m_ImageUVSize = glm::vec2(1.0f) / glm::vec2(x, y);

		CreateDescriptorPool();

		CreateDescriptorSet();
	}
	EditorImageAtlas::~EditorImageAtlas()
	{
		UseContext();

		vkDestroyDescriptorSetLayout(ctx.m_LogicalDevice, m_DescriptorLayout, nullptr);
		vkDestroyDescriptorPool(ctx.m_LogicalDevice, m_DescriptorPool, nullptr);
	}

	ImageUVs EditorImageAtlas::GetImageUVs(uint32_t x, int32_t y)
	{
		const ImVec2 uv0((float)x * m_ImageUVSize.x, (float)y * m_ImageUVSize.y);
		const ImVec2 uv1(uv0.x + m_ImageUVSize.x, uv0.y + m_ImageUVSize.y);

		return { uv0, uv1 };
	}

	void EditorImageAtlas::CreateDescriptorPool()
	{
		UseContext();

		VkDescriptorSetLayoutBinding descriptorBinding{};
		descriptorBinding.binding	      = 0U;
		descriptorBinding.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorBinding.descriptorCount = 1U;
		descriptorBinding.stageFlags	  = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType		= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = 1U;
		layoutInfo.pBindings    = &descriptorBinding;

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorLayout) != VK_SUCCESS)
			EN_ERROR("EditorImageAtlas::CreateDescriptorPool() - Failed to create descriptor set layout!");

		VkDescriptorPoolSize poolSize{};
		poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSize.descriptorCount = 1U;

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = 1U;
		poolInfo.pPoolSizes    = &poolSize;
		poolInfo.maxSets	   = 1U;

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			EN_ERROR("EditorImageAtlas::CreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void EditorImageAtlas::CreateDescriptorSet()
	{
		UseContext();

		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType				 = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool	 = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1U;
		allocInfo.pSetLayouts		 = &m_DescriptorLayout;

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		VkDescriptorImageInfo atlasImageInfo{};
		atlasImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		atlasImageInfo.imageView   = m_Texture->m_Image->m_ImageView;
		atlasImageInfo.sampler     = m_Texture->m_ImageSampler;

		VkWriteDescriptorSet descriptorWrite{};
		descriptorWrite.sType			= VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrite.dstSet			= m_DescriptorSet;
		descriptorWrite.dstBinding		= 0U;
		descriptorWrite.dstArrayElement = 0U;
		descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrite.descriptorCount = 1U;
		descriptorWrite.pImageInfo		= &atlasImageInfo;


		vkUpdateDescriptorSets(ctx.m_LogicalDevice, 1U, &descriptorWrite, 0U, nullptr);
	}
}