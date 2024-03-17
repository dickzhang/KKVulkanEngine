#include "VulkanInstance.h"
VulkanInstance::VulkanInstance()
{
}

VulkanInstance:: ~VulkanInstance()
{
}

VkResult VulkanInstance::createInstance(std::vector<const char *> & layers, std::vector<const char *> & extensionNames, char const * const appName)
{
	//设置实例相关的层和扩展的信息
	layerExtension.appRequestedLayerNames = layers;
	layerExtension.appRequestedExtensionNames = extensionNames;

	//定义wulkan应用程序的结构体
	VkApplicationInfo appInfo = { };
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = appName; //应用的名称
	appInfo.applicationVersion = 1; //应用的版本
	appInfo.pEngineName = appName; //引擎的名称
	appInfo.engineVersion = 1; //引擎的版本
	// VK_API_VERSION is now deprecated, use VK_MAKE_VERSION instead.
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0); //api的版本

	// 定义wulkan实例创建的参数结构体
	VkInstanceCreateInfo instInfo = { };
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	//这个参数可以设置为扩展功能结构体的指针或者为NULL
	instInfo.pNext = &layerExtension.dbgReportCreateInfo; 
	instInfo.flags = 0;
	instInfo.pApplicationInfo = &appInfo;

	// Specify the list of layer name to be enabled.
	instInfo.enabledLayerCount = ( uint32_t )layers.size();
	instInfo.ppEnabledLayerNames = layers.size() ? layers.data() : NULL;

	// Specify the list of extensions to be used in the application.
	instInfo.enabledExtensionCount = ( uint32_t )extensionNames.size();
	instInfo.ppEnabledExtensionNames = extensionNames.size() ? extensionNames.data() : NULL;

	VkResult result = vkCreateInstance(&instInfo, NULL, &instance);
	assert(result == VK_SUCCESS);

	return result;
}

void VulkanInstance::destroyInstance()
{
	vkDestroyInstance(instance, NULL);
}
