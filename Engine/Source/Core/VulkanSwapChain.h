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
	// ������ͼ���������
	VkSurfaceCapabilitiesKHR	surfCapabilities;
	//������չʾģʽ������
	uint32_t					presentModeCount;
	//չʾģʽ������
	std::vector<VkPresentModeKHR> presentModes;
	//����������ɫͼ�ĳߴ�
	VkExtent2D					swapChainExtent;
	//���õ���ɫͼ������
	uint32_t					desiredNumberOfSwapChainImages;

	VkSurfaceTransformFlagBitsKHR preTransform;
	//�����˽�����������Ӧ��չʾģʽ��ʾ(��λ��)
	VkPresentModeKHR			swapchainPresentMode;
	//��ȡ���Ľ�������ɫͼ�����
	std::vector<VkImage>		swapchainImages;
	//�����ʽ����
	std::vector<VkSurfaceFormatKHR> surfFormats;
};

struct SwapChainPublicVariables
{
	//ϵͳƽ̨��ص��߼��������
	VkSurfaceKHR surface;
	//�������õ��Ļ���ͼ�������
	uint32_t swapchainImageCount;
	//����������
	VkSwapchainKHR swapChain;
	//������ͼ����ͼ���б�
	std::vector<SwapChainBuffer> colorBuffer;
	//��ɵ��ź���(һ���ö���֮���ͬ��)
	VkSemaphore presentCompleteSemaphore;
	//��ǰ���õĻ��Ʊ�������
	uint32_t currentColorBuffer;
	//ͼ��ĸ�ʽ
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
	//��ѯ����������չ�Լ�������ַ
	VkResult createSwapChainExtensions();
	//��ѯ��������ͼ���ʽ
	void getSupportedFormats();
	//����չʾ����
	VkResult createSurface();
	//��ȡ֧�ֻ���չʾ��ͼ�ζ���
	uint32_t getGraphicsQueueWithPresentationSupport();
	//��ȡ���������Լ�չʾģʽ
	void getSurfaceCapabilitiesAndPresentMode();
	//����չʾģʽ����Ϣ
	void managePresentMode();
	//��ȡ����������ɫͼ
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
