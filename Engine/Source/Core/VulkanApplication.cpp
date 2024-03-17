#include "VulkanApplication.h"
#include "VulkanDrawable.h"

std::unique_ptr<VulkanApplication> VulkanApplication::instance;
std::once_flag VulkanApplication::onlyOnce;

//extern std::vector<const char *> instanceExtensionNames;
//extern std::vector<const char *> layerNames;
//extern std::vector<const char *> deviceExtensionNames;


//实例扩展名
std::vector<const char *> instanceExtensionNames =
{
	VK_KHR_SURFACE_EXTENSION_NAME,
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,//这个扩展可以打印所有的api调用以及参数和返回值
	VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
};

std::vector<const char *> layerNames =
{
	"VK_LAYER_LUNARG_standard_validation",//按照最佳的顺序加载一组标准的验证层
	//"VK_LAYER_LUNARG_api_dump"//这个层负责将wulkan api的调用信息打印出来
};

//调试报告对应的一些层
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

//设备扩展名
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

	// 查询设备相关的层和扩展
	deviceObj->layerExtension.getDeviceExtensionProperties(gpu);

	// 获取物理设备的属性
	vkGetPhysicalDeviceProperties(*gpu, &deviceObj->gpuProps);

	//查询物理设备内存属性
	vkGetPhysicalDeviceMemoryProperties(*gpu, &deviceObj->memoryProperties);

	//获取物理设备的队列族及其属性
	deviceObj->getPhysicalDeviceQueuesAndProperties();

	//获取图形队列的句柄
	deviceObj->getGrahicsQueueHandle();

	// 创建逻辑设备
	return deviceObj->createDevice(layers, extensions);
}

VkResult VulkanApplication::enumeratePhysicalDevices(std::vector<VkPhysicalDevice> & gpuList)
{
	uint32_t gpuDeviceCount;
	//显示获取物理设备的数量
	VkResult result = vkEnumeratePhysicalDevices(instanceObj.instance, &gpuDeviceCount, NULL);
	assert(result == VK_SUCCESS);

	assert(gpuDeviceCount);

	gpuList.resize(gpuDeviceCount);
	//获取所有物理设备的信息
	result = vkEnumeratePhysicalDevices(instanceObj.instance, &gpuDeviceCount, gpuList.data());
	assert(result == VK_SUCCESS);

	return result;
}

void VulkanApplication::initialize()
{
	char title[] = "WulkanEngine";
	//获取实例所有的层和扩展属性
	instanceObj.layerExtension.getInstanceLayerProperties();
	//显示获取想要打开的层是否支持,然后返回支持的列表
	instanceObj.layerExtension.areLayersSupported(layerNames);
	//创建vulkan实例
	createVulkanInstance(layerNames, instanceExtensionNames, title);

	//是否开启debug调试
	if(debugFlag)
	{
		instanceObj.layerExtension.createDebugReportCallback();
	}

	//获取物理设备列表
	enumeratePhysicalDevices(gpuList);

	// This example use only one device which is available first.
	if(gpuList.size() > 0)
	{
		//和逻辑设备进行握手
		handShakeWithDevice(&gpuList[0], layerNames, deviceExtensionNames);
	}

	if(!rendererObj)
	{
		rendererObj = new VulkanRenderer(this, deviceObj);
		// 创建一个1280x720的展示窗口
		rendererObj->createPresentationWindow(1280, 720);
		// 初始化交换链
		rendererObj->getSwapChain()->intializeSwapChain();
	}
	//渲染的初始化
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
	//这个函数可以确保当前设备上没有任何正在执行的操作再返回
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
	//删除所有流水线对象
	rendererObj->destroyPipeline();

	//删除相关联的流水线缓冲对象
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
