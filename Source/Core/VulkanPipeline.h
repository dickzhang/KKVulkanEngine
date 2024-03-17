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

	//创建流水线缓冲对象,保存流水线对象
	void createPipelineCache();

	//返回创建的流水线对象
	//需要输入一个可绘制对象,获取顶点输入的属性信息,除此之外还有着色器文件,以及一个布尔变量负责检查是否支持深度
	//以及输入的顶点数据是否有效
	bool createPipeline(VulkanDrawable * drawableObj, VkPipeline * pipeline, VulkanShader * shaderObj, VkBool32 includeDepth, VkBool32 includeVi = true);

	//销毁流水线缓冲对象
	void destroyPipelineCache();

public:
	VkPipelineCache	pipelineCache;
	VulkanApplication * appObj;
	VulkanDevice * deviceObj;
};