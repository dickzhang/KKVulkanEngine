#pragma once
#include "Common/Common.h"
#include "Common/Log.h"

#include "Graphics/ModuleBase.h"
#include "Graphics/DVKCommand.h"
#include "Graphics/DVKCamera.h"
#include "Graphics/DVKBuffer.h"
#include "Graphics/DVKPipeline.h"
#include "Graphics/DVKModel.h"
#include "Graphics/ImageGUIContext.h"

#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include <vector>

class PipeLines :public ModuleBase
{
public:
	struct MVPBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct ParamBlock
	{
		float intensity;
	};
public:
	PipeLines(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~PipeLines();
	virtual bool Init() override;
	virtual void Loop(float time,float delta) override;
	virtual void Exist() override;
private:
	void LoadAssets();
	void DestroyAssets();
	void CreateGUI();
	void CreateUniformBuffers();
	void CreateDescriptorSetLayout();
	void CreateDescriptorSet();
	void CreatePipelines();
	void SetupCommandBuffers();

	void DestroyGUI();
	void DestroyDescriptorSetLayout();
	void DestroyPipelines();
	void DestroyUniformBuffers();

	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void UpdateUniformBuffers(float time,float delta);
private:
	typedef std::vector<DVKGfxPipeline*>   DVKPipelines;

	bool                            m_AutoRotate = false;
	bool                            m_Ready = false;

	DVKCamera              m_ViewCamera;

	MVPBlock                        m_MVPData;
	DVKBuffer* m_MVPBuffer;

	ParamBlock                      m_ParamData;
	DVKBuffer* m_ParamBuffer = nullptr;

	DVKPipelines                    m_Pipelines;

	DVKModel* m_Model = nullptr;

	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;

	ImageGUIContext* m_GUI = nullptr;
};
