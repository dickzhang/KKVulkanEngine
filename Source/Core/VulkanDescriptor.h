
#pragma once
#include "../Common/Headers.h"

class VulkanDevice;
class VulkanApplication;

class VulkanDescriptor
{
public:
	VulkanDescriptor();
	~VulkanDescriptor();

	//从描述符池中分配新的描述符
	void createDescriptor(bool useTexture);
	//销毁描述符对象
	void destroyDescriptor();
	//定义描述符集的布局绑定,创建描述符布局
	virtual void createDescriptorSetLayout(bool useTexture) = 0;
	//销毁描述符布局对象
	void destroyDescriptorLayout();
	//创建描述符池
	virtual void createDescriptorPool(bool useTexture) = 0;
	//销毁描述符池
	void destroyDescriptorPool();
	//创建描述符集的资源
	virtual void createDescriptorResources() = 0;
	//从描述符池中分配新的描述符集,并且更新描述符集的属性信息
	virtual void createDescriptorSet(bool useTexture) = 0;
	//销毁描述符集
	void destroyDescriptorSet();

	// 创建流水线布局
	virtual void createPipelineLayout() = 0;
	// 销毁流水线布局
	void destroyPipelineLayouts();

public:
	// 流水线布局对象
	VkPipelineLayout pipelineLayout;

	std::vector<VkDescriptorSetLayout> descLayout;

	//描述符池的对象,用来分配描述符集对象
	VkDescriptorPool descriptorPool;

	// List of all created VkDescriptorSet
	std::vector<VkDescriptorSet> descriptorSet;

	// Logical device used for creating the descriptor pool and descriptor sets
	VulkanDevice * deviceObj;
};