#pragma once

#ifndef EN_SHADER_HPP
#define EN_SHADER_HPP

#include <Renderer/Context.hpp>

namespace en
{
	enum struct ShaderType
	{
		Vertex,
		Geometry,
		Fragment
	};

	class Shader
	{
	public:
		Shader(std::string sourcePath, const ShaderType shaderType);
		~Shader();

		VkPipelineShaderStageCreateInfo m_ShaderInfo{};

		const std::string m_SourcePath;

		static std::vector<char> ReadShaderFile(const std::string path);

	private:
		VkShaderModule m_ShaderModule{};
	};
}

#endif