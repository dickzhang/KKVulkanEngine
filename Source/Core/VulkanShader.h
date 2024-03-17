#pragma once
#include "../Common/Headers.h"

// Shader class managing the shader conversion, compilation, linking
class VulkanShader
{
public:
	// Constructor
	VulkanShader()
	{
	}

	// Destructor
	~VulkanShader()
	{
	}
	//用spv来构架着色器模块
	void buildShaderModuleWithSPV(uint32_t * vertShaderText, size_t vertexSPVSize, uint32_t * fragShaderText, size_t fragmentSPVSize);

	//销毁不再需要的着色器
	void destroyShaders();

#ifdef AUTO_COMPILE_GLSL_TO_SPV
	//转换GLSL着色器到SPIR-V着色器
	bool GLSLtoSPV(const VkShaderStageFlagBits shaderType, const char * pshader, std::vector<unsigned int> & spirv);
	//构建着色器的入口函数
	void buildShader(const char * vertShaderText, const char * fragShaderText);

	//着色器语言类型,这里可以设置为ESHLangVertex
	EShLanguage getLanguage(const VkShaderStageFlagBits shader_type);

	// 初始化TBuitInResource
	void initializeResources(TBuiltInResource & Resources);
#endif
	//用来存储顶点和片元着色器数据
	VkPipelineShaderStageCreateInfo shaderStages[2];
};
