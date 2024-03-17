#pragma once
#include "../Common/Headers.h"
#include "VulkanDescriptor.h"
#include "CommandBufferMgr.h"

class VulkanRenderer;

struct UniformData
{
	VkBuffer						buffer;			// Buffer resource object
	VkDeviceMemory					memory;			// Buffer resourece object's allocated device memory
	VkDescriptorBufferInfo			bufferInfo;		// Buffer info that need to supplied into write descriptor set (VkWriteDescriptorSet)
	VkMemoryRequirements			memRqrmnt;		// Store the queried memory requirement of the uniform buffer
	std::vector<VkMappedMemoryRange>mappedRange;	// Metadata of memory mapped objects
	uint8_t * pData;			// Host pointer containing the mapped device address which is used to write data into.
} ;

//顶点缓存元信息的结构体
struct VertexBuffer
{
	VkBuffer buf;
	VkDeviceMemory mem;
	VkDescriptorBufferInfo bufferInfo;
} ;

class VulkanDrawable : public VulkanDescriptor
{

public:
	VulkanDrawable(VulkanRenderer * parent = 0);
	~VulkanDrawable();
	void createVertexBuffer(const void * vertexData, uint32_t dataSize, uint32_t dataStride, bool useTexture);
	//在渲染之前,先准备可绘制对象,包括分配,创建以及录制指令缓存
	void prepare();
	//渲染可绘制对象
	void render();

	void update();

	void setPipeline(VkPipeline * vulkanPipeline)
	{
		pipeline = vulkanPipeline;
	}
	VkPipeline * getPipeline()
	{
		return pipeline;
	}

	void createUniformBuffer();
	void createDescriptorPool(bool useTexture);
	void createDescriptorResources();
	void createDescriptorSet(bool useTexture);
	void createDescriptorSetLayout(bool useTexture);
	void createPipelineLayout();
	//初始化视口参数
	void initViewports(VkCommandBuffer * cmd);
	//初始化裁剪器参数
	void initScissors(VkCommandBuffer * cmd);
	void destroyVertexBuffer();
	//销毁绘制对象的指令缓存
	void destroyCommandBuffer();
	void destroySynchronizationObjects();
	void destroyUniformBuffer();
	void setTextures(TextureData * tex);
public:
	//负责存储顶点输入数据的遍历方式
	VkVertexInputBindingDescription		viIpBind;
	//负责存储顶点数据解释相关的信息
	VkVertexInputAttributeDescription	viIpAttrb[2];

private:
	//用于绘制的指令缓存
	std::vector<VkCommandBuffer> vecCmdDraw;
	void recordCommandBuffer(int currentImage, VkCommandBuffer * cmdDraw);
	VkViewport viewport;
	VkRect2D   scissor;
	VkSemaphore presentCompleteSemaphore;
	VkSemaphore drawingCompleteSemaphore;
	TextureData * textures;

	glm::mat4 Projection;
	glm::mat4 View;
	glm::mat4 Model;
	glm::mat4 MVP;

	VulkanRenderer * rendererObj;
	VkPipeline * pipeline;
	UniformData m_UniformData;
	VertexBuffer m_VertexBuffer;
};