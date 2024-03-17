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
	//��spv��������ɫ��ģ��
	void buildShaderModuleWithSPV(uint32_t * vertShaderText, size_t vertexSPVSize, uint32_t * fragShaderText, size_t fragmentSPVSize);

	//���ٲ�����Ҫ����ɫ��
	void destroyShaders();

#ifdef AUTO_COMPILE_GLSL_TO_SPV
	//ת��GLSL��ɫ����SPIR-V��ɫ��
	bool GLSLtoSPV(const VkShaderStageFlagBits shaderType, const char * pshader, std::vector<unsigned int> & spirv);
	//������ɫ������ں���
	void buildShader(const char * vertShaderText, const char * fragShaderText);

	//��ɫ����������,�����������ΪESHLangVertex
	EShLanguage getLanguage(const VkShaderStageFlagBits shader_type);

	// ��ʼ��TBuitInResource
	void initializeResources(TBuiltInResource & Resources);
#endif
	//�����洢�����ƬԪ��ɫ������
	VkPipelineShaderStageCreateInfo shaderStages[2];
};
