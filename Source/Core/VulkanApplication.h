#pragma once
#include "VulkanInstance.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include "VulkanLayerAndExtension.h"
//是一个单例类
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

	// 创建wulkan的实例对象
	VkResult createVulkanInstance(std::vector<const char *> & layers, std::vector<const char *> & extensions, const char * applicationName);

	//负责创建逻辑设备以及相关的队列.
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
	//是否开启调试功能
	bool debugFlag=false;
	std::vector<VkPhysicalDevice> gpuList;
};