#include "../Common/Headers.h"
#include "../Core/VulkanApplication.h"

int main(int argc, char ** argv)
{
	//������һ������application
	VulkanApplication * appObj = VulkanApplication::GetInstance();
	//���г�ʼ��
	appObj->initialize();
	//׼��
	appObj->prepare();
	bool isWindowOpen = true;
	std::cout << argv << std::endl;
	while(isWindowOpen)
	{
		//��Ⱦ֮ǰ�����ݸ���
		appObj->update();
		//������Ⱦ
		isWindowOpen = appObj->render();
	}
	//�ͷ�
	appObj->deInitialize();
	return 0;
}
