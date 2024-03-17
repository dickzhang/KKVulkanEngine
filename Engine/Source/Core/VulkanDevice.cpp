#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanApplication.h"

VulkanDevice::VulkanDevice(VkPhysicalDevice * physicalDevice)
{
	gpu = physicalDevice;
}

VulkanDevice::~VulkanDevice()
{
}

VkResult VulkanDevice::createDevice(std::vector<const char *> & layers, std::vector<const char *> & extensions)
{
	layerExtension.appRequestedLayerNames = layers;
	layerExtension.appRequestedExtensionNames = extensions;

	// Create Device with available queue information.
	VkResult result;
	float queuePriorities[1] = {0.0};
	VkDeviceQueueCreateInfo queueInfo = { };
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.pNext = NULL;
	//图形队列在队列族中的位置索引
	queueInfo.queueFamilyIndex = graphicsQueueIndex; 
	//要创建的队列族的数量
	queueInfo.queueCount = 1;
	//设个参数是一个归一化之后的浮点数的数组,它设置了用户对创建后的每个队列的工作优先级要求
	queueInfo.pQueuePriorities = queuePriorities;

	//获取了gpu设备的所有特征
	vkGetPhysicalDeviceFeatures(*gpu, &deviceFeatures);
	//创建一个gpu设备特征对象,然后都设置false
	VkPhysicalDeviceFeatures setEnabledFeatures = {VK_FALSE};
	//只是把采样器的各向异性同步了一下
	setEnabledFeatures.samplerAnisotropy = deviceFeatures.samplerAnisotropy;

	VkDeviceCreateInfo deviceInfo = { };
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.pNext = NULL;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledLayerCount = 0;
	deviceInfo.ppEnabledLayerNames = NULL;											// Device layers are deprecated
	deviceInfo.enabledExtensionCount = ( uint32_t )extensions.size();
	deviceInfo.ppEnabledExtensionNames = extensions.size() ? extensions.data() : NULL;
	deviceInfo.pEnabledFeatures = &setEnabledFeatures;

	result = vkCreateDevice(*gpu, &deviceInfo, NULL, &device);
	assert(result == VK_SUCCESS);
	return result;
}

bool VulkanDevice::memoryTypeFromProperties(uint32_t typeBits, VkFlags requirementsMask, uint32_t * typeIndex)
{
	for(uint32_t i = 0; i < 32; i++)
	{
		if(( typeBits & 1 ) == 1)
		{
			if(( memoryProperties.memoryTypes[i].propertyFlags & requirementsMask ) == requirementsMask)
			{
				*typeIndex = i;
				return true;
			}
		}
		typeBits >>= 1;
	}
	return false;
}

void VulkanDevice::getPhysicalDeviceQueuesAndProperties()
{
	//获取gpu上队列的数量
	vkGetPhysicalDeviceQueueFamilyProperties(*gpu, &queueFamilyCount, NULL);

	queueFamilyProps.resize(queueFamilyCount);
	//获取gpu队列的属性信息
	vkGetPhysicalDeviceQueueFamilyProperties(*gpu, &queueFamilyCount, queueFamilyProps.data());
}

uint32_t VulkanDevice::getGrahicsQueueHandle()
{
	//	1. Get the number of Queues supported by the Physical device
	//	2. Get the properties each Queue type or Queue Family
	//			There could be 4 Queue type or Queue families supported by physical device - 
	//			Graphics Queue	- VK_QUEUE_GRAPHICS_BIT 
	//			Compute Queue	- VK_QUEUE_COMPUTE_BIT
	//			DMA				- VK_QUEUE_TRANSFER_BIT
	//			Spare memory	- VK_QUEUE_SPARSE_BINDING_BIT
	//	3. Get the index ID for the required Queue family, this ID will act like a handle index to queue.

	bool found = false;
	// 1. Iterate number of Queues supported by the Physical device
	for(unsigned int i = 0; i < queueFamilyCount; i++)
	{
		// 2. Get the Graphics Queue type
		//		There could be 4 Queue type or Queue families supported by physical device - 
		//		Graphics Queue		- VK_QUEUE_GRAPHICS_BIT 
		//		Compute Queue		- VK_QUEUE_COMPUTE_BIT
		//		DMA/Transfer Queue	- VK_QUEUE_TRANSFER_BIT
		//		Spare memory		- VK_QUEUE_SPARSE_BINDING_BIT

		if(queueFamilyProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
		{
			// 3. Get the handle/index ID of graphics queue family.
			found = true;
			graphicsQueueIndex = i;
			break;
		}
	}

	// Assert if there is no queue found.
	assert(found);

	return 0;
}

void VulkanDevice::destroyDevice()
{
	vkDestroyDevice(device, NULL);
}

void VulkanDevice::getDeviceQueue()
{  //根据队列的索引获取队列
	vkGetDeviceQueue(device, graphicsQueueWithPresentIndex, 0, &queue);
}
