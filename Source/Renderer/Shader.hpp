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
		Shader(std::string shaderPath, const ShaderType& shaderType);
		~Shader();

		VkPipelineShaderStageCreateInfo m_ShaderInfo{};

		const std::string& GetPath() const { return m_Path; }

	private:
		VkShaderModule m_ShaderModule{};

		std::string m_Path;

		static std::vector<char> ReadShaderFile(std::string shaderPath);
	};
}

#endif