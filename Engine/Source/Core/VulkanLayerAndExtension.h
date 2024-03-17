#pragma once
#include "../Common/Headers.h"

//定义一个层对应的扩展属性结构体
struct LayerProperties
{
	VkLayerProperties properties;
	std::vector<VkExtensionProperties> extensions;
};

class VulkanLayerAndExtension
{
public:
	VulkanLayerAndExtension();
	~VulkanLayerAndExtension();

	//负责查询实例/全局的层
	VkResult getInstanceLayerProperties();
	//获取全局的扩展
	VkResult getExtensionProperties(LayerProperties & layerProps, VkPhysicalDevice * gpu = NULL);
	//获取基于设备的扩展
	VkResult getDeviceExtensionProperties(VkPhysicalDevice * gpu);

	//判断一个层设备是否支持
	VkBool32 areLayersSupported(std::vector<const char *> & layerNames);

	VkResult createDebugReportCallback();
	void	destroyDebugReportCallback();

	static VKAPI_ATTR VkBool32 VKAPI_CALL debugFunction(VkFlags msgFlags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t srcObject,
		size_t location,
		int32_t msgCode,
		const char * layerPrefix,
		const char * msg,
		void * userData);

private:
	PFN_vkCreateDebugReportCallbackEXT dbgCreateDebugReportCallback;
	PFN_vkDestroyDebugReportCallbackEXT dbgDestroyDebugReportCallback;
	VkDebugReportCallbackEXT debugReportCallback;
public:
	VkDebugReportCallbackCreateInfoEXT dbgReportCreateInfo = { };
	// 应用需要开启的层名称列表
	std::vector<const char *>			appRequestedLayerNames;
	// 应用需要开启的扩展名称列表
	std::vector<const char *>			appRequestedExtensionNames;
	//层和对应的扩展属性列表
	std::vector<LayerProperties>		layerPropertyList;
};