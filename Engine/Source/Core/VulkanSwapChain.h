#pragma once
#include "../Common/Headers.h"

class VulkanInstance;
class VulkanDevice;
class VulkanRenderer;
class VulkanApplication;

struct SwapChainBuffer
{
	VkImage image;
	VkImageView view;
};

struct SwapChainPrivateVariables
{
	// 保存了图像表面属性
	VkSurfaceCapabilitiesKHR	surfCapabilities;
	//保存了展示模式的数量
	uint32_t					presentModeCount;
	//展示模式的数组
	std::vector<VkPresentModeKHR> presentModes;
	//交换链中颜色图的尺寸
	VkExtent2D					swapChainExtent;
	//可用的颜色图的数量
	uint32_t					desiredNumberOfSwapChainImages;

	VkSurfaceTransformFlagBitsKHR preTransform;
	//保存了交换链创建对应的展示模式表示(按位与)
	VkPresentModeKHR			swapchainPresentMode;
	//获取到的交换链颜色图像对象
	std::vector<VkImage>		swapchainImages;
	//表面格式数组
	std::vector<VkSurfaceFormatKHR> surfFormats;
};

struct SwapChainPublicVariables
{
	//系统平台相关的逻辑表面对象
	VkSurfaceKHR surface;
	//交换链用到的缓存图像的数量
	uint32_t swapchainImageCount;
	//交换链对象
	VkSwapchainKHR swapChain;
	//交换链图像视图的列表
	std::vector<SwapChainBuffer> colorBuffer;
	//完成的信号量(一般用队列之间的同步)
	VkSemaphore presentCompleteSemaphore;
	//当前可用的绘制表面索引
	uint32_t currentColorBuffer;
	//图像的格式
	VkFormat format;
};

class VulkanSwapChain
{
public:
	VulkanSwapChain(VulkanRenderer * renderer);
	~VulkanSwapChain();
	void intializeSwapChain();
	void createSwapChain(const VkCommandBuffer & cmd);
	void destroySwapChain();
	void setSwapChainExtent(uint32_t swapChainWidth, uint32_t swapChainHeight);

private:
	//查询交换链的扩展以及函数地址
	VkResult createSwapChainExtensions();
	//查询交换链的图像格式
	void getSupportedFormats();
	//场景展示表面
	VkResult createSurface();
	//获取支持画面展示的图形队列
	uint32_t getGraphicsQueueWithPresentationSupport();
	//获取表面属性以及展示模式
	void getSurfaceCapabilitiesAndPresentMode();
	//管理展示模式的信息
	void managePresentMode();
	//获取交换链的颜色图
	void createSwapChainAndGetColorImages();
	void createColorImageView(const VkCommandBuffer & cmd);

public:
	SwapChainPublicVariables	scPublicVars;
	PFN_vkQueuePresentKHR		fpQueuePresentKHR;
	PFN_vkAcquireNextImageKHR	fpAcquireNextImageKHR;
private:
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR		fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR	fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR		fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR	fpGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkDestroySurfaceKHR							fpDestroySurfaceKHR;
	PFN_vkCreateSwapchainKHR	fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR	fpDestroySwapchainKHR;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;

	SwapChainPrivateVariables	scPrivateVars;
	VulkanRenderer * rendererObj;	// parent
	VulkanApplication * appObj;
};
