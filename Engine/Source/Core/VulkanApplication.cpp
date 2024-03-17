#include "VulkanApplication.h"
#include "VulkanDrawable.h"

std::unique_ptr<VulkanApplication> VulkanApplication::instance;
std::once_flag VulkanApplication::onlyOnce;

//extern std::vector<const char *> instanceExtensionNames;
//extern std::vector<const char *> layerNames;
//extern std::vector<const char *> deviceExtensionNames;


//ʵ����չ��
std::vector<const char *> instanceExtensionNames =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,//�����չ���Դ�ӡ���е�api�����Լ������ͷ���ֵ
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

std::vector<const char *> layerNames =
{
	"VK_LAYER_LUNARG_standard_validation",//������ѵ�˳�����һ���׼����֤��
	//"VK_LAYER_LUNARG_api_dump"//����㸺��wulkan api�ĵ�����Ϣ��ӡ����
};

//���Ա����Ӧ��һЩ��
//std::vector<const char *> layerNames =
//{
//	"VK_LAYER_GOOGLE_threading",
//	"VK_LAYER_LUNARG_parameter_validation",
//	"VK_LAYER_LUNARG_device_limits",
//	"VK_LAYER_LUNARG_object_tracker",
//	"VK_LAYER_LUNARG_image",
//	"VK_LAYER_LUNARG_core_validation",
//	"VK_LAYER_LUNARG_core_swapchain",
//	"VK_LAYER_LUNARG_core_unique_objects"
//};

//�豸��չ��
std::vector<const char *> deviceExtensionNames =
{
	VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

VulkanApplication::VulkanApplication()
{
}

VulkanApplication::~VulkanApplication()
{
	if(rendererObj)
	{
		delete rendererObj;
		rendererObj = NULL;
	}
}

VulkanApplication * VulkanApplication::GetInstance()
{
	std::call_once(onlyOnce, [] ()
	{
		instance.reset(new VulkanApplication());
	});
	return instance.get();
}

VkResult VulkanApplication::createVulkanInstance(std::vector<const char *> & layers, std::vector<const char *> & extensionNames, const char * applicationName)
{
	return instanceObj.createInstance(layers, extensionNames, applicationName);
}

VkResult VulkanApplication::handShakeWithDevice(VkPhysicalDevice * gpu, std::vector<const char *> & layers, std::vector<const char *> & extensions)
{
	deviceObj = new VulkanDevice(gpu);
	if(!deviceObj)
	{
		return VK_ERROR_OUT_OF_HOST_MEMORY;
	}

	// ��ѯ�豸��صĲ����չ
	deviceObj->layerExtension.getDeviceExtensionProperties(gpu);

	// ��ȡ�����豸������
	vkGetPhysicalDeviceProperties(*gpu, &deviceObj->gpuProps);

	//��ѯ�����豸�ڴ�����
	vkGetPhysicalDeviceMemoryProperties(*gpu, &deviceObj->memoryProperties);

	//��ȡ�����豸�Ķ����弰������
	deviceObj->getPhysicalDeviceQueuesAndProperties();

	//��ȡͼ�ζ��еľ��
	deviceObj->getGrahicsQueueHandle();

	// �����߼��豸
	return deviceObj->createDevice(layers, extensions);
}

VkResult VulkanApplication::enumeratePhysicalDevices(std::vector<VkPhysicalDevice> & gpuList)
{
	uint32_t gpuDeviceCount;
	//��ʾ��ȡ�����豸������
	VkResult result = vkEnumeratePhysicalDevices(instanceObj.instance, &gpuDeviceCount, NULL);
	assert(result == VK_SUCCESS);

	assert(gpuDeviceCount);

	gpuList.resize(gpuDeviceCount);
	//��ȡ���������豸����Ϣ
	result = vkEnumeratePhysicalDevices(instanceObj.instance, &gpuDeviceCount, gpuList.data());
	assert(result == VK_SUCCESS);

	return result;
}

void VulkanApplication::initialize()
{
	char title[] = "WulkanEngine";
	//��ȡʵ�����еĲ����չ����
	instanceObj.layerExtension.getInstanceLayerProperties();
	//��ʾ��ȡ��Ҫ�򿪵Ĳ��Ƿ�֧��,Ȼ�󷵻�֧�ֵ��б�
	instanceObj.layerExtension.areLayersSupported(layerNames);
	//����vulkanʵ��
	createVulkanInstance(layerNames, instanceExtensionNames, title);

	//�Ƿ���debug����
	if(debugFlag)
	{
		instanceObj.layerExtension.createDebugReportCallback();
	}

	//��ȡ�����豸�б�
	enumeratePhysicalDevices(gpuList);

	// This example use only one device which is available first.
	if(gpuList.size() > 0)
	{
		//���߼��豸��������
		handShakeWithDevice(&gpuList[0], layerNames, deviceExtensionNames);
	}

	if(!rendererObj)
	{
		rendererObj = new VulkanRenderer(this, deviceObj);
		// ����һ��1280x720��չʾ����
		rendererObj->createPresentationWindow(1280, 720);
		// ��ʼ��������
		rendererObj->getSwapChain()->intializeSwapChain();
	}
	//��Ⱦ�ĳ�ʼ��
	rendererObj->initialize();
}

void VulkanApplication::resize()
{
	// If prepared then only proceed for 
	if(!isPrepared)
	{
		return;
	}

	isResizing = true;
	//�����������ȷ����ǰ�豸��û���κ�����ִ�еĲ����ٷ���
	vkDeviceWaitIdle(deviceObj->device);
	rendererObj->destroyFramebuffers();
	rendererObj->destroyCommandPool();
	rendererObj->destroyPipeline();
	rendererObj->getPipelineObject()->destroyPipelineCache();
	for each(VulkanDrawable * drawableObj in *rendererObj->getDrawingItems())
	{
		drawableObj->destroyDescriptor();
	}
	rendererObj->destroyRenderpass();
	rendererObj->getSwapChain()->destroySwapChain();
	rendererObj->destroyDrawableVertexBuffer();
	rendererObj->destroyDrawableUniformBuffer();
	rendererObj->destroyTextureResource();
	rendererObj->destroyDepthBuffer();
	rendererObj->initialize();
	prepare();
	isResizing = false;
}

void VulkanApplication::deInitialize()
{
	//ɾ��������ˮ�߶���
	rendererObj->destroyPipeline();

	//ɾ�����������ˮ�߻������
	rendererObj->getPipelineObject()->destroyPipelineCache();

	for each(VulkanDrawable * drawableObj in *rendererObj->getDrawingItems())
	{
		drawableObj->destroyDescriptor();
	}

	rendererObj->getShader()->destroyShaders();
	rendererObj->destroyFramebuffers();
	rendererObj->destroyRenderpass();
	rendererObj->destroyDrawableVertexBuffer();
	rendererObj->destroyDrawableUniformBuffer();

	rendererObj->destroyDrawableCommandBuffer();
	rendererObj->destroyDepthBuffer();
	rendererObj->getSwapChain()->destroySwapChain();
	rendererObj->destroyCommandBuffer();
	rendererObj->destroyDrawableSynchronizationObjects();
	rendererObj->destroyCommandPool();
	rendererObj->destroyPresentationWindow();
	rendererObj->destroyTextureResource();
	deviceObj->destroyDevice();
	if(debugFlag)
	{
		instanceObj.layerExtension.destroyDebugReportCallback();
	}
	instanceObj.destroyInstance();
}

void VulkanApplication::prepare()
{
	isPrepared = false;
	rendererObj->prepare();
	isPrepared = true;
}

void VulkanApplication::update()
{
	rendererObj->update();
}

bool VulkanApplication::render()
{
	if(!isPrepared)
		return false;

	return rendererObj->render();
}
