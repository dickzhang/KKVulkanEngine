#pragma once
#include "../Common/Headers.h"
#include "VulkanLayerAndExtension.h"
class VulkanApplication;

class VulkanDevice
{
public:
	VulkanDevice(VkPhysicalDevice * gpu);
	~VulkanDevice();

	//�����߼��豸
	VkResult createDevice(std::vector<const char *> & layers, std::vector<const char *> & extensions);
	void destroyDevice();

	//�ж�ͼƬ�ڴ����ʱ����ʵ��ڴ�����
	bool memoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t * typeIndex);

	// ��ȡ�����豸���к�����
	void getPhysicalDeviceQueuesAndProperties();

	// ��ȡͼ�ζ��еľ��
	uint32_t getGrahicsQueueHandle();

	//��ȡ�豸����
	void getDeviceQueue();

public:
	// �߼��豸
	VkDevice device;
	// �����豸
	VkPhysicalDevice * gpu;
	VkPhysicalDeviceProperties			gpuProps;	// �����豸������
	VkPhysicalDeviceMemoryProperties	memoryProperties; //�����豸���ڴ�����

	VkQueue									queue;							// ���ж���
	std::vector<VkQueueFamilyProperties>	queueFamilyProps;				// ������������б�
	uint32_t								graphicsQueueIndex;				// ͼ�ζ��е�����
	uint32_t								graphicsQueueWithPresentIndex;  // ֧��ͼ��չʾ��ͼ�ζ�������
	uint32_t								queueFamilyCount;				// �����������

	VulkanLayerAndExtension		layerExtension; //�����չ����
	VkPhysicalDeviceFeatures	deviceFeatures; //�����豸������
};