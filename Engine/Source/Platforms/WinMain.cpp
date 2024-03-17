#include "../Common/Headers.h"
#include "../Core/VulkanApplication.h"

int main(int argc, char ** argv)
{
	//创建了一个单例application
	VulkanApplication * appObj = VulkanApplication::GetInstance();
	//进行初始化
	appObj->initialize();
	//准备
	appObj->prepare();
	bool isWindowOpen = true;
	std::cout << argv << std::endl;
	while(isWindowOpen)
	{
		//渲染之前的数据更新
		appObj->update();
		//进行渲染
		isWindowOpen = appObj->render();
	}
	//释放
	appObj->deInitialize();
	return 0;
}
