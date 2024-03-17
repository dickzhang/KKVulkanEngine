#pragma once
#include "../Common/Headers.h"

//����һ�����Ӧ����չ���Խṹ��
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

	//�����ѯʵ��/ȫ�ֵĲ�
	VkResult getInstanceLayerProperties();
	//��ȡȫ�ֵ���չ
	VkResult getExtensionProperties(LayerProperties & layerProps, VkPhysicalDevice * gpu = NULL);
	//��ȡ�����豸����չ
	VkResult getDeviceExtensionProperties(VkPhysicalDevice * gpu);

	//�ж�һ�����豸�Ƿ�֧��
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
	// Ӧ����Ҫ�����Ĳ������б�
	std::vector<const char *>			appRequestedLayerNames;
	// Ӧ����Ҫ��������չ�����б�
	std::vector<const char *>			appRequestedExtensionNames;
	//��Ͷ�Ӧ����չ�����б�
	std::vector<LayerProperties>		layerPropertyList;
};