#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/ModuleBase.h"
#include "Graphics/DVKCommand.h"
#include "Graphics/DVKCamera.h"
#include "Graphics/DVKBuffer.h"
#include "Math/Vector4.h"
#include "Graphics/DVKModel.h"
#include "Math/Matrix4x4.h"
#include "Graphics/ImageGUIContext.h"
#include <vector>

class LoadMesh :public ModuleBase
{
public:
	struct UBOData
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};
public:
	LoadMesh(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~LoadMesh();
	virtual bool Init() override;
	virtual void Loop(float time,float delta) override;
	virtual void Exist() override;
private:
	void LoadAssets();
	void CreateGUI();
	void CreateUniformBuffers();
	void CreateDescriptorPool();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreatePipelines();
	void SetupCommandBuffers();
	void DestroyAssets();
	void DestroyGUI();
	void DestroyDescriptorSetLayout();
	void DestroyDescriptorPool();
	void DestroyPipelines();
	void DestroyUniformBuffers();
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void UpdateUniformBuffers(float time,float delta);
private:
	typedef std::vector<DVKBuffer*>    DVKBuffers;
	typedef std::vector<VkDescriptorSet> VkDescriptorSets;
	bool                            m_AutoRotate = false;
	bool                            m_Ready = false;
	std::vector<UBOData>            m_MVPDatas;
	DVKBuffers                      m_MVPBuffers;
	DVKCamera              m_ViewCamera;
	DVKModel* m_Model = nullptr;
	VkPipeline                      m_Pipeline = VK_NULL_HANDLE;
	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSets                m_DescriptorSets;
	ImageGUIContext* m_GUI = nullptr;
};

