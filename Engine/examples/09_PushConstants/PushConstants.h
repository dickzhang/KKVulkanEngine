#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Graphics/DVKCamera.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include <vector>

class PushConstants :public ModuleBase
{
public:
	struct ViewProjectionBlock
	{
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct ModelPushConstantBlock
	{
		Matrix4x4 model;
	};
public:
	PushConstants(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~PushConstants();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
private:
	void Draw(float time,float delta);

	void CreateGUI();
	bool UpdateUI(float time,float delta);
	void DestroyGUI();

	void LoadAssets();
	void DestroyAssets();

	void UpdateUniformBuffers(float time,float delta);
	void SetupCommandBuffers();

	void CreateUniformBuffers();
	void DestroyUniformBuffers();

	void CreateDescriptorSetLayout();
	void DestroyDescriptorSetLayout();
	void CreateDescriptorSet();

	void CreatePipelines();
	void DestroyPipelines();
	

private:
	bool                            m_AutoRotate = false;
	bool                            m_Ready = false;
	DVKCamera              m_ViewCamera;
	ViewProjectionBlock             m_ViewProjData;
	DVKBuffer* m_ViewProjBuffer = nullptr;
	DVKGfxPipeline* m_Pipeline = nullptr;
	DVKModel* m_Model = nullptr;
	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
	ImageGUIContext* m_GUI = nullptr;
};