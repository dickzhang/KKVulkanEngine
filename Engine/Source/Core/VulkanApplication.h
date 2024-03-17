#pragma once
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanLayerAndExtension.h"
//��һ��������
class VulkanApplication
{
public:
	// DTOR
	~VulkanApplication();
	static VulkanApplication * GetInstance();
	void initialize();				// Initialize and allocate resources
	void prepare();					// Prepare resource
	void update();					// Update data
	void resize();					// Resize window
	bool render();					// Render primitives
	void deInitialize();			// Release resources

private:
	VulkanApplication();

	// ����wulkan��ʵ������
	VkResult createVulkanInstance(std::vector<const char *> & layers, std::vector<const char *> & extensions, const char * applicationName);

	//���𴴽��߼��豸�Լ���صĶ���.
	VkResult handShakeWithDevice(VkPhysicalDevice * gpu, std::vector<const char *> & layers, std::vector<const char *> & extensions);

	VkResult enumeratePhysicalDevices(std::vector<VkPhysicalDevice> & gpus);

public:
	VulkanInstance instanceObj;	
	VulkanDevice * deviceObj=nullptr;
	VulkanRenderer * rendererObj=nullptr;
	bool isPrepared=false;
	bool isResizing=false;

private:
	static std::unique_ptr<VulkanApplication> instance;
	static std::once_flag onlyOnce;
	//�Ƿ������Թ���
	bool debugFlag=false;
	std::vector<VkPhysicalDevice> gpuList;
};