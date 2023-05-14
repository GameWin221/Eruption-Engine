#include "Sampler.hpp"

namespace en
{
    Sampler::Sampler(VkFilter filter, uint32_t anisotropy, float maxLod, float mipLodBias)
    {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(Context::Get().m_PhysicalDevice, &properties);

        VkSamplerCreateInfo samplerInfo{
            .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,

            .magFilter = filter,
            .minFilter = filter,

            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,

            .addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            .addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,

            .mipLodBias = mipLodBias,

            .anisotropyEnable = (anisotropy > 0.0f),
            .maxAnisotropy = std::fminf(anisotropy, properties.limits.maxSamplerAnisotropy),

            .compareEnable = VK_FALSE,
            .compareOp = VK_COMPARE_OP_ALWAYS,

            .minLod = 0.0f,
            .maxLod = maxLod,

            .borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
            .unnormalizedCoordinates = VK_FALSE,
        };

        if (vkCreateSampler(Context::Get().m_LogicalDevice, &samplerInfo, nullptr, &m_Handle) != VK_SUCCESS)
            EN_ERROR("Sampler::Sampler() - Failed to create a sampler!");
    }

    Sampler::~Sampler()
    {
        vkDestroySampler(Context::Get().m_LogicalDevice, m_Handle, nullptr);
    }
}