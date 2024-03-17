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

//���㻺��Ԫ��Ϣ�Ľṹ��
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
	//����Ⱦ֮ǰ,��׼���ɻ��ƶ���,��������,�����Լ�¼��ָ���
	void prepare();
	//��Ⱦ�ɻ��ƶ���
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
	//��ʼ���ӿڲ���
	void initViewports(VkCommandBuffer * cmd);
	//��ʼ���ü�������
	void initScissors(VkCommandBuffer * cmd);
	void destroyVertexBuffer();
	//���ٻ��ƶ����ָ���
	void destroyCommandBuffer();
	void destroySynchronizationObjects();
	void destroyUniformBuffer();
	void setTextures(TextureData * tex);
public:
	//����洢�����������ݵı�����ʽ
	VkVertexInputBindingDescription		viIpBind;
	//����洢�������ݽ�����ص���Ϣ
	VkVertexInputAttributeDescription	viIpAttrb[2];

private:
	//���ڻ��Ƶ�ָ���
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