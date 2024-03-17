#pragma once
#include "../Common/Headers.h"
#include "VulkanLayerAndExtension.h"
class VulkanApplication;

class VulkanDevice
{
public:
	VulkanDevice(VkPhysicalDevice * gpu);
	~VulkanDevice();

	//创建逻辑设备
	VkResult createDevice(std::vector<const char *> & layers, std::vector<const char *> & extensions);
	void destroyDevice();

	//判断图片内存分配时最合适的内存类型
	bool memoryTypeFromProperties(uint32_t typeBits, VkFlags requirements_mask, uint32_t * typeIndex);

	// 获取物理设备队列和属性
	void getPhysicalDeviceQueuesAndProperties();

	// 获取图形队列的句柄
	uint32_t getGrahicsQueueHandle();

	//获取设备队列
	void getDeviceQueue();

public:
	// 逻辑设备
	VkDevice device;
	// 物理设备
	VkPhysicalDevice * gpu;
	VkPhysicalDeviceProperties			gpuProps;	// 物理设备的属性
	VkPhysicalDeviceMemoryProperties	memoryProperties; //物理设备的内存属性

	VkQueue									queue;							// 队列对象
	std::vector<VkQueueFamilyProperties>	queueFamilyProps;				// 队列族的属性列表
	uint32_t								graphicsQueueIndex;				// 图形队列的索引
	uint32_t								graphicsQueueWithPresentIndex;  // 支持图形展示的图形队列索引
	uint32_t								queueFamilyCount;				// 队列族的数量

	VulkanLayerAndExtension		layerExtension; //层和扩展对象
	VkPhysicalDeviceFeatures	deviceFeatures; //物理设备的特性
};