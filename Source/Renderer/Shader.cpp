#include <Core/EnPch.hpp>
#include "Shader.hpp"

namespace en
{
    Shader::Shader(std::string shaderPath, const ShaderType& shaderType)
    {
        auto shaderCode = Shader::ReadShaderFile(shaderPath);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        UseContext();

        if (vkCreateShaderModule(ctx.m_LogicalDevice, &createInfo, nullptr, &m_ShaderModule) != VK_SUCCESS)
            EN_ERROR("Shader.cpp::Shader::Shader() - Failed to create shader module!");

        VkShaderStageFlagBits shaderStage;
        switch (shaderType)
        {
        case ShaderType::Vertex:
            shaderStage = VK_SHADER_STAGE_VERTEX_BIT;
            break;
        case ShaderType::Geometry:
            shaderStage = VK_SHADER_STAGE_GEOMETRY_BIT;
            break;
        case ShaderType::Fragment:
            shaderStage = VK_SHADER_STAGE_FRAGMENT_BIT;
            break;
        }

        m_ShaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        m_ShaderInfo.stage = shaderStage;
        m_ShaderInfo.module = m_ShaderModule;
        m_ShaderInfo.pName = "main";
    }

    Shader::~Shader()
    {
        UseContext();

        vkDestroyShaderModule(ctx.m_LogicalDevice, m_ShaderModule, nullptr);
    }

    std::vector<char> Shader::ReadShaderFile(std::string shaderPath)
    {
        std::ifstream file(shaderPath, std::ios::ate | std::ios::binary);

        if (!file.is_open())
            EN_ERROR("Shader.cpp::Shader::ReadShaderFile() - Failed to open shader source file!");

        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();

        return buffer;
    }
}