#pragma once
#include "../Common/Headers.h"
#include "VulkanSwapChain.h"
#include "VulkanDrawable.h"
#include "VulkanShader.h"
#include "VulkanPipeline.h"

#define NUM_SAMPLES VK_SAMPLE_COUNT_1_BIT

class VulkanRenderer
{
public:
	VulkanRenderer(VulkanApplication * app, VulkanDevice * deviceObject);
	~VulkanRenderer();

public:
	void initialize();
	void prepare();
	void update();
	bool render();

	void createPresentationWindow(const int & windowWidth = 500, const int & windowHeight = 500);
	//����ͼ�񲼾�Ϊ���/ģ���Ż�����
	void setImageLayout(VkImage image, VkImageAspectFlags aspectMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, const VkImageSubresourceRange & subresourceRange, const VkCommandBuffer & cmdBuf);

	//���ڻص����������������е��¼�
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	// Destroy the presentation window
	void destroyPresentationWindow();

	// Getter functions for member variable specific to classes.
	inline VulkanApplication * getApplication()
	{
		return application;
	}
	inline VulkanDevice * getDevice()
	{
		return deviceObj;
	}
	inline VulkanSwapChain * getSwapChain()
	{
		return swapChainObj;
	}
	inline std::vector<VulkanDrawable *> * getDrawingItems()
	{
		return &drawableList;
	}
	inline VkCommandPool * getCommandPool()
	{
		return &cmdPool;
	}
	inline VulkanShader * getShader()
	{
		return &shaderObj;
	}
	inline VulkanPipeline * getPipelineObject()
	{
		return &pipelineObj;
	}

	void createCommandPool();							// Create command pool
	void buildSwapChainAndDepthImage();					// Create swapchain color image and depth image
	void createDepthImage();							// Create depth image
	void createVertexBuffer();
	//������Ⱦͨ��
	void createRenderPass(bool includeDepth, bool clear = true);	// Render Pass creation
	//����֡����
	void createFrameBuffer(bool includeDepth);
	//����shaderģ��
	void createShaders();
	//������ˮ��
	void createPipelineStateManagement();
	//����������
	void createDescriptors();
	//��������ƽ�̵�����
	void createTextureLinear(const char * filename, TextureData * texture, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
	//�����Ż�ƽ�̵�����
	void createTextureOptimal(const char * filename, TextureData * texture, VkImageUsageFlags imageUsageFlags = VK_IMAGE_USAGE_SAMPLED_BIT, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);

	//����ָ���
	void destroyCommandBuffer();
	//����ָ����
	void destroyCommandPool();
	void destroyDepthBuffer();
	void destroyDrawableVertexBuffer();
	void destroyRenderpass();										// Destroy the render pass object when no more required
	void destroyFramebuffers();
	void destroyPipeline();
	void destroyDrawableCommandBuffer();
	void destroyDrawableSynchronizationObjects();
	void destroyDrawableUniformBuffer();
	void destroyTextureResource();
public:
#ifdef _WIN32
#define APP_NAME_STR_LEN 80
	HINSTANCE					connection;				// hInstance - Windows Instance
	char						name[APP_NAME_STR_LEN]; // name - App name appearing on the window
	HWND						window;					// hWnd - the window handle
#else
	xcb_connection_t * connection;
	xcb_screen_t * screen;
	xcb_window_t				window;
	xcb_intern_atom_reply_t * reply;
#endif

	struct
	{
		VkFormat		format;
		VkImage			image;
		VkDeviceMemory	mem;
		VkImageView		view;
	}Depth;
	//���ͼ����ָ��
	VkCommandBuffer		cmdDepthImage;			// Command buffer for depth image layout
	//ָ����
	VkCommandPool		cmdPool;				// Command pool
	//���㻺��ָ��
	VkCommandBuffer		cmdVertexBuffer;		// Command buffer for vertex buffer - Triangle geometry
	VkCommandBuffer		cmdTexture;				// Command buffer for creating the texture

	VkRenderPass		renderPass;				// Render pass created object
	std::vector<VkFramebuffer> framebuffers;	// Number of frame buffer corresponding to each swap chain
	std::vector<VkPipeline *> pipelineList;		// List of pipelines

	int					width, height;
	TextureData			texture;

private:
	VulkanApplication * application;
	VulkanDevice * deviceObj;
	VulkanSwapChain * swapChainObj;
	std::vector<VulkanDrawable *> drawableList;
	VulkanShader 	   shaderObj;
	VulkanPipeline 	   pipelineObj;
};