#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class InputAttachments :public ModuleBase
{
public:
	struct ModelBlock
	{
		Matrix4x4 model;
	};

	struct ViewProjectionBlock
	{
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct AttachmentParamBlock
	{
		int attachmentIndex;
	};

public:
	InputAttachments(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~InputAttachments();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void CreateFrameBuffers() override;
	void DestroyFrameBuffers() override;
	void CreateRenderPass() override;
	void DestoryRenderPass() override;
	void CreateDepthStencil() override;
	void DestoryDepthStencil() override;
	void CreateAttachments();

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

	void CreateDescriptorSet();

	void CreatePipelines();
	void DestroyPipelines();

	void DestroyAttachments();

private:
	typedef std::vector<DVKDescriptorSet*>     DVKDescriptorSetArray;
	typedef std::vector<DVKTexture*>           DVKTextureArray;

	bool                            m_Ready = false;

	DVKCamera              m_ViewCamera;

	std::vector<uint8>              m_ModelDatas;
	DVKBuffer* m_ModelBuffer = nullptr;
	VkDescriptorBufferInfo          m_ModelBufferInfo;

	DVKBuffer* m_ViewProjBuffer = nullptr;
	ViewProjectionBlock             m_ViewProjData;

	DVKBuffer* m_DebugBuffer = nullptr;
	AttachmentParamBlock            m_DebugParam;
	std::vector<const char*>        m_DebugNames;

	DVKModel* m_Model = nullptr;
	DVKModel* m_Quad = nullptr;

	DVKGfxPipeline* m_Pipeline0 = nullptr;
	DVKShader* m_Shader0 = nullptr;
	DVKDescriptorSet* m_DescriptorSet0 = nullptr;

	DVKGfxPipeline* m_Pipeline1 = nullptr;
	DVKShader* m_Shader1 = nullptr;
	DVKDescriptorSetArray           m_DescriptorSets;

	DVKTextureArray                 m_AttachsDepth;
	DVKTextureArray                 m_AttachsColor;
	DVKTextureArray                 m_AttachsNormal;
	ImageGUIContext* m_GUI = nullptr;
	float m_zNear=0.1;
	float m_zFar=1000.0;
};
