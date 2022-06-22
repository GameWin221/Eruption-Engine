#include "Core/EnPch.hpp"
#include "EditorImageAtlas.hpp"

namespace en
{
	EditorImageAtlas::EditorImageAtlas(const std::string& atlasPath, const uint32_t& x, const uint32_t& y)
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

	ImageUVs EditorImageAtlas::GetImageUVs(const uint32_t& x, const uint32_t& y)
	{
		const ImVec2 uv0(x	   * m_ImageUVSize.x, y		* m_ImageUVSize.y);
		const ImVec2 uv1(uv0.x + m_ImageUVSize.x, uv0.y + m_ImageUVSize.y);

		return { uv0, uv1 };
	}

	void EditorImageAtlas::CreateDescriptorPool()
	{
		UseContext();

		constexpr VkDescriptorSetLayoutBinding descriptorBinding{
			.binding		 = 0U,
			.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1U,
			.stageFlags		 = VK_SHADER_STAGE_FRAGMENT_BIT
		};

		const VkDescriptorSetLayoutCreateInfo layoutInfo{
			.sType		  = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = 1U,
			.pBindings	  = &descriptorBinding
		};

		if (vkCreateDescriptorSetLayout(ctx.m_LogicalDevice, &layoutInfo, nullptr, &m_DescriptorLayout) != VK_SUCCESS)
			EN_ERROR("EditorImageAtlas::CreateDescriptorPool() - Failed to create descriptor set layout!");

		constexpr VkDescriptorPoolSize poolSize{
			.type			 = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1U
		};

		const VkDescriptorPoolCreateInfo poolInfo{
			.sType		   = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets	   = 1U,
			.poolSizeCount = 1U,
			.pPoolSizes	   = &poolSize
		};

		if (vkCreateDescriptorPool(ctx.m_LogicalDevice, &poolInfo, nullptr, &m_DescriptorPool) != VK_SUCCESS)
			EN_ERROR("EditorImageAtlas::CreateDescriptorPool() - Failed to create descriptor pool!");
	}
	void EditorImageAtlas::CreateDescriptorSet()
	{
		UseContext();

		const VkDescriptorSetAllocateInfo allocInfo{
			.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool		= m_DescriptorPool,
			.descriptorSetCount = 1U,
			.pSetLayouts		= &m_DescriptorLayout,
		};

		if (vkAllocateDescriptorSets(ctx.m_LogicalDevice, &allocInfo, &m_DescriptorSet) != VK_SUCCESS)
			EN_ERROR("Material::CreateDescriptorSet() - Failed to allocate descriptor sets!");

		const VkDescriptorImageInfo atlasImageInfo{
			.sampler     = m_Texture->m_ImageSampler,
			.imageView   = m_Texture->m_Image->m_ImageView,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		const VkWriteDescriptorSet descriptorWrite{
			.sType			 = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
			.dstSet			 = m_DescriptorSet,
			.dstBinding		 = 0U,
			.dstArrayElement = 0U,
			.descriptorCount = 1U,
			.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.pImageInfo		 = &atlasImageInfo
		};

		vkUpdateDescriptorSets(ctx.m_LogicalDevice, 1U, &descriptorWrite, 0U, nullptr);
	}
}