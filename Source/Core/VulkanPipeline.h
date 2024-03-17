#pragma once
#include "../Common/Headers.h"
class VulkanShader;
class VulkanDrawable;
class VulkanDevice;
class VulkanApplication;

#define NUMBER_OF_VIEWPORTS 1
#define NUMBER_OF_SCISSORS NUMBER_OF_VIEWPORTS

class VulkanPipeline
{
public:
	VulkanPipeline();

	~VulkanPipeline();

	//������ˮ�߻������,������ˮ�߶���
	void createPipelineCache();

	//���ش�������ˮ�߶���
	//��Ҫ����һ���ɻ��ƶ���,��ȡ���������������Ϣ,����֮�⻹����ɫ���ļ�,�Լ�һ�����������������Ƿ�֧�����
	//�Լ�����Ķ��������Ƿ���Ч
	bool createPipeline(VulkanDrawable * drawableObj, VkPipeline * pipeline, VulkanShader * shaderObj, VkBool32 includeDepth, VkBool32 includeVi = true);

	//������ˮ�߻������
	void destroyPipelineCache();

public:
	VkPipelineCache	pipelineCache;
	VulkanApplication * appObj;
	VulkanDevice * deviceObj;
};