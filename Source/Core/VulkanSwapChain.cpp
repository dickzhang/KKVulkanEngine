#include "VulkanSwapChain.h"

#include "VulkanDevice.h"
#include "VulkanInstance.h"
#include "VulkanRenderer.h"
#include "VulkanApplication.h"

#define INSTANCE_FUNC_PTR(instance, entrypoint){											\
    fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(instance, "vk"#entrypoint); \
    if (fp##entrypoint == NULL) {															\
        std::cout << "Unable to locate the vkGetDeviceProcAddr: vk"#entrypoint;				\
        exit(-1);																			\
    }																						\
}

#define DEVICE_FUNC_PTR(dev, entrypoint){													\
    fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);		\
    if (fp##entrypoint == NULL) {															\
        std::cout << "Unable to locate the vkGetDeviceProcAddr: vk"#entrypoint;				\
        exit(-1);																			\
    }																						\
}

VulkanSwapChain::VulkanSwapChain(VulkanRenderer * renderer)
{
	rendererObj = renderer;
	appObj = VulkanApplication::GetInstance();
	scPublicVars.swapChain = VK_NULL_HANDLE;
}

VulkanSwapChain::~VulkanSwapChain()
{
	scPrivateVars.swapchainImages.clear();
	scPrivateVars.surfFormats.clear();
	scPrivateVars.presentModes.clear();
}

VkResult VulkanSwapChain::createSwapChainExtensions()
{
	// Dependency on createPresentationWindow()
	VkInstance & instance = appObj->instanceObj.instance;
	VkDevice & device = appObj->deviceObj->device;

	// Get Instance based swap chain extension function pointer
	INSTANCE_FUNC_PTR(instance, GetPhysicalDeviceSurfaceSupportKHR);
	INSTANCE_FUNC_PTR(instance, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	INSTANCE_FUNC_PTR(instance, GetPhysicalDeviceSurfaceFormatsKHR);
	INSTANCE_FUNC_PTR(instance, GetPhysicalDeviceSurfacePresentModesKHR);
	INSTANCE_FUNC_PTR(instance, DestroySurfaceKHR);

	// Get Device based swap chain extension function pointer
	DEVICE_FUNC_PTR(device, CreateSwapchainKHR);
	DEVICE_FUNC_PTR(device, DestroySwapchainKHR);
	DEVICE_FUNC_PTR(device, GetSwapchainImagesKHR);
	DEVICE_FUNC_PTR(device, AcquireNextImageKHR);
	DEVICE_FUNC_PTR(device, QueuePresentKHR);

	return VK_SUCCESS;
}

VkResult VulkanSwapChain::createSurface()
{
	VkResult  result;
	VkInstance & instance = appObj->instanceObj.instance;
#ifdef _WIN32
	VkWin32SurfaceCreateInfoKHR createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.hinstance = rendererObj->connection;
	createInfo.hwnd = rendererObj->window;
	result = vkCreateWin32SurfaceKHR(instance, &createInfo, NULL, &scPublicVars.surface);

#else  // linux
	VkXcbSurfaceCreateInfoKHR createInfo = { };
	createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	createInfo.pNext = NULL;
	createInfo.connection = rendererObj->connection;
	createInfo.window = rendererObj->window;
	result = vkCreateXcbSurfaceKHR(instance, &createInfo, NULL, &surface);
#endif 
	assert(result == VK_SUCCESS);
	return result;
}

uint32_t VulkanSwapChain::getGraphicsQueueWithPresentationSupport()
{
	VulkanDevice * device = appObj->deviceObj;
	uint32_t queueCount = device->queueFamilyCount;
	VkPhysicalDevice gpu = *device->gpu;
	std::vector<VkQueueFamilyProperties> & queueProps = device->queueFamilyProps;

	//�������ÿ������,�ж�չʾ����״̬
	VkBool32 * supportsPresent = ( VkBool32 * )malloc(queueCount * sizeof(VkBool32));
	for(uint32_t i = 0; i < queueCount; i++)
	{
		fpGetPhysicalDeviceSurfaceSupportKHR(gpu, i, scPublicVars.surface, &supportsPresent[i]);
	}
	//���ͼ�ζ����Ƿ�֧����չ����
	uint32_t graphicsQueueNodeIndex = UINT32_MAX;
	uint32_t presentQueueNodeIndex = UINT32_MAX;
	for(uint32_t i = 0; i < queueCount; i++)
	{
		if(( queueProps[i].queueFlags & VK_QUEUE_GRAPHICS_BIT ) != 0)
		{
			if(graphicsQueueNodeIndex == UINT32_MAX)
			{
				graphicsQueueNodeIndex = i;
			}

			if(supportsPresent[i] == VK_TRUE)
			{
				graphicsQueueNodeIndex = i;
				presentQueueNodeIndex = i;
				break;
			}
		}
	}
	//���û���ҵ�һ��ͬʱ֧��ͼ�κ�չʾ�Ķ���,��ô����Ѱ��չʾ����
	if(presentQueueNodeIndex == UINT32_MAX)
	{
		for(uint32_t i = 0; i < queueCount; ++i)
		{
			if(supportsPresent[i] == VK_TRUE)
			{
				presentQueueNodeIndex = i;
				break;
			}
		}
	}

	free(supportsPresent);
	//���û���ҵ�ͼ�ζ��к�չʾ���������ʾ����
	if(graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX)
	{
		return  UINT32_MAX;
	}
	return graphicsQueueNodeIndex;
}

void VulkanSwapChain::getSupportedFormats()
{
	VkPhysicalDevice gpu = *rendererObj->getDevice()->gpu;
	VkResult  result;

	//��ȡ֧�ֵĸ�ʽ����
	uint32_t formatCount;
	result = fpGetPhysicalDeviceSurfaceFormatsKHR(gpu, scPublicVars.surface, &formatCount, NULL);
	assert(result == VK_SUCCESS);
	scPrivateVars.surfFormats.clear();
	scPrivateVars.surfFormats.resize(formatCount);

	// ��ȡ֧�ֵı����ʽ����
	result = fpGetPhysicalDeviceSurfaceFormatsKHR(gpu, scPublicVars.surface, &formatCount, &scPrivateVars.surfFormats[0]);
	assert(result == VK_SUCCESS);

	//�����һ��,����û�ж����,���Ǿ�Ĭ����VK_FORMAT_B8G8R8A8_UNORM��ʽ
	if(formatCount == 1 && scPrivateVars.surfFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		scPublicVars.format = VK_FORMAT_B8G8R8A8_UNORM;
	}
	else
	{
		assert(formatCount >= 1);
		scPublicVars.format = scPrivateVars.surfFormats[0].format;
	}
}

void VulkanSwapChain::intializeSwapChain()
{
	// ��ѯ����������չ�����ĵ�ַ
	createSwapChainExtensions();

	//����չʾ����
	createSurface();

	//��ȡ֧�ֻ���չʾ��ͼ�ζ���
	uint32_t index = getGraphicsQueueWithPresentationSupport();
	if(index == UINT32_MAX)
	{
		std::cout << "Could not find a graphics and a present queue\nCould not find a graphics and a present queue\n";
		exit(-1);
	}
	rendererObj->getDevice()->graphicsQueueWithPresentIndex = index;
	//��ȡ֧�ֵĸ�ʽ
	getSupportedFormats();
}

void VulkanSwapChain::createSwapChain(const VkCommandBuffer & cmd)
{
	//��ȡ���������������,�Լ�չʾģʽ
	getSurfaceCapabilitiesAndPresentMode();
	//����չʾģʽ
	managePresentMode();
	//�����������Լ���ȡ�������е�ͼ�λ���
	createSwapChainAndGetColorImages();
	//������ɫͼ��ͼ
	createColorImageView(cmd);
}

void VulkanSwapChain::getSurfaceCapabilitiesAndPresentMode()
{
	VkResult  result;
	VkPhysicalDevice gpu = *appObj->deviceObj->gpu;
	result = fpGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, scPublicVars.surface, &scPrivateVars.surfCapabilities);
	assert(result == VK_SUCCESS);

	result = fpGetPhysicalDeviceSurfacePresentModesKHR(gpu, scPublicVars.surface, &scPrivateVars.presentModeCount, NULL);
	assert(result == VK_SUCCESS);

	scPrivateVars.presentModes.clear();
	scPrivateVars.presentModes.resize(scPrivateVars.presentModeCount);
	assert(scPrivateVars.presentModes.size() >= 1);

	result = fpGetPhysicalDeviceSurfacePresentModesKHR(gpu, scPublicVars.surface, &scPrivateVars.presentModeCount, &scPrivateVars.presentModes[0]);
	assert(result == VK_SUCCESS);

	if(scPrivateVars.surfCapabilities.currentExtent.width == ( uint32_t )-1)
	{
		//���û�ж������Ŀ��,����������Ϊ��ͼƬһ���ĳߴ�
		scPrivateVars.swapChainExtent.width = rendererObj->width;
		scPrivateVars.swapChainExtent.height = rendererObj->height;
	}
	else
	{
		//��������˱���ߴ�,��ô������ͽ������ĳߴ����Ǻ�
		scPrivateVars.swapChainExtent = scPrivateVars.surfCapabilities.currentExtent;
	}
}


void VulkanSwapChain::managePresentMode()
{
	scPrivateVars.swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	for(size_t i = 0; i < scPrivateVars.presentModeCount; i++)
	{
		if(scPrivateVars.presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
		{
			scPrivateVars.swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
			break;
		}
		if(( scPrivateVars.swapchainPresentMode != VK_PRESENT_MODE_MAILBOX_KHR ) &&
			( scPrivateVars.presentModes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR ))
		{
			scPrivateVars.swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
		}
	}

	scPrivateVars.desiredNumberOfSwapChainImages = scPrivateVars.surfCapabilities.minImageCount + 1;
	if(( scPrivateVars.surfCapabilities.maxImageCount > 0 ) &&
		( scPrivateVars.desiredNumberOfSwapChainImages > scPrivateVars.surfCapabilities.maxImageCount ))
	{
		scPrivateVars.desiredNumberOfSwapChainImages = scPrivateVars.surfCapabilities.maxImageCount;
	}

	if(scPrivateVars.surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		scPrivateVars.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		scPrivateVars.preTransform = scPrivateVars.surfCapabilities.currentTransform;
	}
}

void VulkanSwapChain::createSwapChainAndGetColorImages()
{
	VkResult  result;
	VkSwapchainKHR oldSwapchain = scPublicVars.swapChain;

	VkSwapchainCreateInfoKHR swapChainInfo = { };
	swapChainInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapChainInfo.pNext = NULL;
	swapChainInfo.surface = scPublicVars.surface;
	swapChainInfo.minImageCount = scPrivateVars.desiredNumberOfSwapChainImages;
	swapChainInfo.imageFormat = scPublicVars.format;
	swapChainInfo.imageExtent.width = scPrivateVars.swapChainExtent.width;
	swapChainInfo.imageExtent.height = scPrivateVars.swapChainExtent.height;
	swapChainInfo.preTransform = scPrivateVars.preTransform;//չʾʱ,ͼ����ʾ����任�Ƕ�
	swapChainInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;//͸��ͨ���Ļ��ģʽ
	swapChainInfo.imageArrayLayers = 1;//������������˶�����ͼ������,��ͼ������
	swapChainInfo.presentMode = scPrivateVars.swapchainPresentMode;
	swapChainInfo.oldSwapchain = oldSwapchain;
	swapChainInfo.clipped = true;
	swapChainInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	swapChainInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	swapChainInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;//������ͼ��Ĺ���ģʽ
	swapChainInfo.queueFamilyIndexCount = 0;
	swapChainInfo.pQueueFamilyIndices = NULL;
	//ͨ����չ����ָ�봴��������
	result = fpCreateSwapchainKHR(rendererObj->getDevice()->device, &swapChainInfo, NULL, &scPublicVars.swapChain);
	assert(result == VK_SUCCESS); 

	// ��ȡ�������а�����ͼ������
	result = fpGetSwapchainImagesKHR(rendererObj->getDevice()->device, scPublicVars.swapChain, &scPublicVars.swapchainImageCount, NULL);
	assert(result == VK_SUCCESS);

	scPrivateVars.swapchainImages.clear();
	scPrivateVars.swapchainImages.resize(scPublicVars.swapchainImageCount);
	assert(scPrivateVars.swapchainImages.size() >= 1);

	// ��ȡ�������е�ͼƬ����
	result = fpGetSwapchainImagesKHR(rendererObj->getDevice()->device, scPublicVars.swapChain, &scPublicVars.swapchainImageCount, &scPrivateVars.swapchainImages[0]);
	assert(result == VK_SUCCESS);

	if(oldSwapchain != VK_NULL_HANDLE)
	{//���پɵĽ�����
		fpDestroySwapchainKHR(rendererObj->getDevice()->device, oldSwapchain, NULL);
	}
}

void VulkanSwapChain::createColorImageView(const VkCommandBuffer & cmd)
{
	VkResult  result;
	scPublicVars.colorBuffer.clear();
	for(uint32_t i = 0; i < scPublicVars.swapchainImageCount; i++)
	{
		SwapChainBuffer sc_buffer;
		VkImageViewCreateInfo imgViewInfo = { };
		imgViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imgViewInfo.pNext = NULL;
		imgViewInfo.format = scPublicVars.format;
		imgViewInfo.components = {VK_COMPONENT_SWIZZLE_IDENTITY};
		imgViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imgViewInfo.subresourceRange.baseMipLevel = 0;
		imgViewInfo.subresourceRange.levelCount = 1;
		imgViewInfo.subresourceRange.baseArrayLayer = 0;
		imgViewInfo.subresourceRange.layerCount = 1;
		imgViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imgViewInfo.flags = 0;

		sc_buffer.image = scPrivateVars.swapchainImages[i];
		imgViewInfo.image = sc_buffer.image;
		result = vkCreateImageView(rendererObj->getDevice()->device, &imgViewInfo, NULL, &sc_buffer.view);
		scPublicVars.colorBuffer.push_back(sc_buffer);
		assert(result == VK_SUCCESS);
	}
	scPublicVars.currentColorBuffer = 0;
}

void VulkanSwapChain::destroySwapChain()
{
	VulkanDevice * deviceObj = appObj->deviceObj;

	for(uint32_t i = 0; i < scPublicVars.swapchainImageCount; i++)
	{
		vkDestroyImageView(deviceObj->device, scPublicVars.colorBuffer[i].view, NULL);
	}

	if(!appObj->isResizing)
	{
		// This piece code will only executes at application shutdown.
		// During resize the old swapchain image is delete in createSwapChainColorImages()
		fpDestroySwapchainKHR(deviceObj->device, scPublicVars.swapChain, NULL);
		vkDestroySurfaceKHR(appObj->instanceObj.instance, scPublicVars.surface, NULL);
	}
}

void VulkanSwapChain::setSwapChainExtent(uint32_t swapChainWidth, uint32_t swapChainHeight)
{
	scPrivateVars.swapChainExtent.width = swapChainWidth;
	scPrivateVars.swapChainExtent.height = swapChainHeight;
}
