#include "../Common/Headers.h"
#include "../Core/VulkanApplication.h"

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

int main(int argc, char ** argv)
{
	//������һ������application
	VulkanApplication * appObj = VulkanApplication::GetInstance();
	//���г�ʼ��
	appObj->initialize();
	//׼��
	appObj->prepare();
	bool isWindowOpen = true;
	while(isWindowOpen)
	{
		//��Ⱦ֮ǰ�����ݸ���
		appObj->update();
		//������Ⱦ
		isWindowOpen = appObj->render();
	}
	//�ͷ�
	appObj->deInitialize();
}
