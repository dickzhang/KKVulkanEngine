#include "VulkanInstance.h"
VulkanInstance::VulkanInstance()
{
}

VulkanInstance:: ~VulkanInstance()
{
}

VkResult VulkanInstance::createInstance(std::vector<const char *> & layers, std::vector<const char *> & extensionNames, char const * const appName)
{
	//����ʵ����صĲ����չ����Ϣ
	layerExtension.appRequestedLayerNames = layers;
	layerExtension.appRequestedExtensionNames = extensionNames;

	//����wulkanӦ�ó���Ľṹ��
	VkApplicationInfo appInfo = { };
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pNext = NULL;
	appInfo.pApplicationName = appName; //Ӧ�õ�����
	appInfo.applicationVersion = 1; //Ӧ�õİ汾
	appInfo.pEngineName = appName; //���������
	appInfo.engineVersion = 1; //����İ汾
	// VK_API_VERSION is now deprecated, use VK_MAKE_VERSION instead.
	appInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0); //api�İ汾

	// ����wulkanʵ�������Ĳ����ṹ��
	VkInstanceCreateInfo instInfo = { };
	instInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	//���������������Ϊ��չ���ܽṹ���ָ�����ΪNULL
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
