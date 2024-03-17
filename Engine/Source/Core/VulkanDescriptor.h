
#pragma once
#include "../Common/Headers.h"

class VulkanDevice;
class VulkanApplication;

class VulkanDescriptor
{
public:
	VulkanDescriptor();
	~VulkanDescriptor();

	//�����������з����µ�������
	void createDescriptor(bool useTexture);
	//��������������
	void destroyDescriptor();
	//�������������Ĳ��ְ�,��������������
	virtual void createDescriptorSetLayout(bool useTexture) = 0;
	//�������������ֶ���
	void destroyDescriptorLayout();
	//������������
	virtual void createDescriptorPool(bool useTexture) = 0;
	//������������
	void destroyDescriptorPool();
	//����������������Դ
	virtual void createDescriptorResources() = 0;
	//�����������з����µ���������,���Ҹ�������������������Ϣ
	virtual void createDescriptorSet(bool useTexture) = 0;
	//������������
	void destroyDescriptorSet();

	// ������ˮ�߲���
	virtual void createPipelineLayout() = 0;
	// ������ˮ�߲���
	void destroyPipelineLayouts();

public:
	// ��ˮ�߲��ֶ���
	VkPipelineLayout pipelineLayout;

	std::vector<VkDescriptorSetLayout> descLayout;

	//�������صĶ���,��������������������
	VkDescriptorPool descriptorPool;

	// List of all created VkDescriptorSet
	std::vector<VkDescriptorSet> descriptorSet;

	// Logical device used for creating the descriptor pool and descriptor sets
	VulkanDevice * deviceObj;
};