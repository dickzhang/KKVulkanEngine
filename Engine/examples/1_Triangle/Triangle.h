#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Application/AppModuleBase.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "File/FileManager.h"
#include "Vulkan/VulkanCommon.h"
#include <vector>

class TriangleModule : public AppModuleBase
{
public:
	struct GPUBuffer
	{
		VkDeviceMemory  memory = VK_NULL_HANDLE;
		VkBuffer        buffer = VK_NULL_HANDLE;
		GPUBuffer() = default;
	};
	struct Vertex
	{
		float position[3];
		float color[3];
	};
	struct UBOData
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};
	typedef GPUBuffer   IndexBuffer;
	typedef GPUBuffer   VertexBuffer;
	typedef GPUBuffer   UBOBuffer;

public:
	TriangleModule(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	virtual ~TriangleModule();

	virtual bool PreInit() override;
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;

protected:
	virtual void CreateFrameBuffers() override;

	virtual void CreateDepthStencil() override;

	virtual void CreateRenderPass() override;

	virtual void DestroyFrameBuffers() override;

	virtual void DestoryRenderPass() override;

	virtual void DestoryDepthStencil() override;

private:
	VkShaderModule LoadSPIPVShader(const std::string& filepath);

	void Draw(float time,float delta);

	void SetupCommandBuffers();

	void CreateDescriptorSet();

	void CreateDescriptorPool();

	void DestroyDescriptorPool();

	void CreatePipelines();

	void DestroyPipelines();

	void CreateDescriptorSetLayout();

	void DestroyDescriptorSetLayout();

	void CreateCommandBuffers();

	void DestroyCommandBuffers();

	void UpdateUniformBuffers(float time,float delta);

	void CreateUniformBuffers();

	void DestroyUniformBuffers();

	void CreateMeshBuffers();

	void DestroyMeshBuffers();

	void CreateSemaphores();

	void DestorySemaphores();

	void CreateFences();

	void DestroyFences();

private:
	bool                            m_Ready = false;
	uint32                          m_IndicesCount = 0;
	std::vector<VkFramebuffer>      m_FrameBuffers;

	VkImage                         m_DepthStencilImage = VK_NULL_HANDLE;
	VkImageView                     m_DepthStencilView = VK_NULL_HANDLE;
	VkDeviceMemory                  m_DepthStencilMemory = VK_NULL_HANDLE;

	VkRenderPass                    m_RenderPass = VK_NULL_HANDLE;
	VkSampleCountFlagBits           m_SampleCount = VK_SAMPLE_COUNT_1_BIT;
	PixelFormat                     m_DepthFormat = PF_D24;

	VkCommandPool                   m_CommandPool = VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>    m_CommandBuffers;

	std::vector<VkFence>            m_Fences;
	VkSemaphore                     m_PresentComplete = VK_NULL_HANDLE;
	VkSemaphore                     m_RenderComplete = VK_NULL_HANDLE;

	VertexBuffer                    m_VertexBuffer;
	IndexBuffer                     m_IndicesBuffer;
	UBOBuffer                       m_MVPBuffer;
	UBOData                         m_MVPData;

	VkDescriptorBufferInfo          m_MVPDescriptor;

	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;

	VkPipeline                      m_Pipeline = VK_NULL_HANDLE;
	VkPipelineCache                 m_PipelineCache = VK_NULL_HANDLE;


};
