#pragma once

#ifndef EN_SHADER_HPP
#define EN_SHADER_HPP

#include <Rendering/Context.hpp>

namespace en
{
	enum struct ShaderType
	{
		VertexShader,
		GeometryShader,
		FragmentShader
	};

	class Shader
	{
	public:
		Shader(std::string shaderPath, const ShaderType& shaderType);
		~Shader();

		VkShaderModule m_ShaderModule;
		VkPipelineShaderStageCreateInfo m_ShaderInfo;

		static std::vector<char> ReadShaderFile(std::string shaderPath);
	};
}

#endif