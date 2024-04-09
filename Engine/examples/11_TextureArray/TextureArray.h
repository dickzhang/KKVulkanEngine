#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include <vector>

class TextureArray :public ModuleBase
{
public:
	struct ModelBlock
	{
		Matrix4x4 model;
	};

	struct ParamBlock
	{
		float step;
		float debug;
		float padding0;
		float padding1;
	};

	struct ViewProjectionBlock
	{
		Matrix4x4 view;
		Matrix4x4 projection;
	};

public:
	TextureArray(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~TextureArray();
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
	std::vector<uint8>              m_ModelDatas;
	DVKBuffer* m_ModelBuffer = nullptr;
	ParamBlock                      m_ParamData;
	DVKBuffer* m_ParamBuffer = nullptr;
	DVKBuffer* m_ViewProjBuffer = nullptr;
	ViewProjectionBlock             m_ViewProjData;
	DVKGfxPipeline* m_Pipeline = nullptr;
	DVKTexture* m_Texture = nullptr;
	DVKModel* m_Model = nullptr;
	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
	ImageGUIContext* m_GUI = nullptr;
};
