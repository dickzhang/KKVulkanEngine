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
	//ͼ�ζ����ڶ������е�λ������
	queueInfo.queueFamilyIndex = graphicsQueueIndex; 
	//Ҫ�����Ķ����������
	queueInfo.queueCount = 1;
	//���������һ����һ��֮��ĸ�����������,���������û��Դ������ÿ�����еĹ������ȼ�Ҫ��
	queueInfo.pQueuePriorities = queuePriorities;

	//��ȡ��gpu�豸����������
	vkGetPhysicalDeviceFeatures(*gpu, &deviceFeatures);
	//����һ��gpu�豸��������,Ȼ������false
	VkPhysicalDeviceFeatures setEnabledFeatures = {VK_FALSE};
	//ֻ�ǰѲ������ĸ�������ͬ����һ��
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
	//��ȡgpu�϶��е�����
	vkGetPhysicalDeviceQueueFamilyProperties(*gpu, &queueFamilyCount, NULL);

	queueFamilyProps.resize(queueFamilyCount);
	//��ȡgpu���е�������Ϣ
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
{  //���ݶ��е�������ȡ����
	vkGetDeviceQueue(device, graphicsQueueWithPresentIndex, 0, &queue);
}
