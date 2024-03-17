#pragma once
#include "VulkanLayerAndExtension.h"

class VulkanInstance
{
public:
	VulkanInstance();

	~VulkanInstance();

	//����wulkanʵ��
	VkResult createInstance(std::vector<const char *> & layers, std::vector<const char *> & extensions, const char * applicationName);
	//����wulkanʵ��
	void destroyInstance();

public:
	//wulkanʵ��
	VkInstance	instance;
	//�����չ����
	VulkanLayerAndExtension		layerExtension;
};
