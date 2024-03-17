#pragma once
#include "VulkanLayerAndExtension.h"

class VulkanInstance
{
public:
	VulkanInstance();

	~VulkanInstance();

	//创建wulkan实例
	VkResult createInstance(std::vector<const char *> & layers, std::vector<const char *> & extensions, const char * applicationName);
	//销毁wulkan实例
	void destroyInstance();

public:
	//wulkan实例
	VkInstance	instance;
	//层和扩展对象
	VulkanLayerAndExtension		layerExtension;
};
