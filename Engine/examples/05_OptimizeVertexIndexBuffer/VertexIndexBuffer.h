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
#include "Graphics/DVKIndexBuffer.h"
#include "Graphics/DVKVertexBuffer.h"
#include <vector>

class VertexIndexBuffer :public ModuleBase
{
public:
	struct UBOData
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

public:
	VertexIndexBuffer(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~VertexIndexBuffer();
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
	bool                            m_AutoRotate = false;
	bool                            m_Ready = false;
	DVKCamera              m_ViewCamera;
	UBOData                         m_MVPData;
	DVKIndexBuffer* m_IndexBuffer = nullptr;
	DVKVertexBuffer* m_VertexBuffer = nullptr;
	DVKBuffer* m_MVPBuffer = nullptr;
	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkPipeline                      m_Pipeline = VK_NULL_HANDLE;
	ImageGUIContext* m_GUI = nullptr;
};

