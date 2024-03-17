#include "../Common/Headers.h"
#include "../Core/VulkanApplication.h"

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

int main(int argc, char ** argv)
{
	//创建了一个单例application
	VulkanApplication * appObj = VulkanApplication::GetInstance();
	//进行初始化
	appObj->initialize();
	//准备
	appObj->prepare();
	bool isWindowOpen = true;
	while(isWindowOpen)
	{
		//渲染之前的数据更新
		appObj->update();
		//进行渲染
		isWindowOpen = appObj->render();
	}
	//释放
	appObj->deInitialize();
}
