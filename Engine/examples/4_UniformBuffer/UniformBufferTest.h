#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/ModuleBase.h"
#include "Graphics/DVKBuffer.h"
#include "Graphics/DVKCommand.h"
#include "Graphics/DVKUtils.h"
#include "Graphics/DVKCamera.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "File/FileManager.h"
#include "Graphics/ImageGUIContext.h"
#include <vector>

class UniformBufferTest :public ModuleBase
{
public:
	struct Vertex
	{
		float position[3];
		float uv[2];
	};

	struct UBOMVPData
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct UBOParams
	{
		float omega;
		float k;
		float cutoff;
		float padding;
	};

public:
	UniformBufferTest(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);

	~UniformBufferTest();
	virtual bool Init() override;
	virtual void Loop(float time,float delta) override;
	virtual void Exist() override;
private:
	void CreateGUI();
	bool UpdateUI(float time,float delta);
	void CreateMeshBuffers();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreatePipelines();
	void SetupCommandBuffers();
	void Draw(float time,float delta);
	void DestroyGUI();
	void DestroyDescriptorSetLayout();
	void DestroyDescriptorPool();
	void DestroyPipelines();
	void DestroyUniformBuffers();
	void DestroyMeshBuffers();
	void UpdateUniformBuffers(float time,float delta);
private:
	bool                            m_Ready = false;
	DVKCamera                       m_ViewCamera;
	UBOMVPData                      m_MVPData;
	UBOParams                       m_Params;
	DVKBuffer* m_VertexBuffer = nullptr;
	DVKBuffer* m_IndexBuffer = nullptr;
	DVKBuffer* m_MVPBuffer = nullptr;
	DVKBuffer* m_ParamsBuffer = nullptr;

	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkPipeline                      m_Pipeline = VK_NULL_HANDLE;
	uint32                          m_IndicesCount = 0;
	ImageGUIContext* m_GUI = nullptr;
};

