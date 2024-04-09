#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class Texture3D :public ModuleBase
{
public:
	struct MVPBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct LutDebugBlock
	{
		float bias;
		float padding0;
		float padding1;
		float padding2;
	};

	struct ImageInfo
	{
		int32   width = 0;
		int32   height = 0;
		int32   comp = 0;
		uint8* data = nullptr;
	};

public:
	Texture3D(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~Texture3D();
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
	bool m_Ready = false;
	DVKCamera  m_ViewCamera;
	MVPBlock   m_MVPData;
	DVKBuffer* m_MVPBuffer;
	LutDebugBlock m_LutDebugData;
	DVKBuffer* m_LutDebugBuffer = nullptr;
	DVKTexture* m_TexOrigin = nullptr;
	DVKTexture* m_Tex3DLut = nullptr;
	DVKGfxPipeline* m_Pipeline0 = nullptr;
	DVKGfxPipeline* m_Pipeline1 = nullptr;
	DVKGfxPipeline* m_Pipeline2 = nullptr;
	DVKGfxPipeline* m_Pipeline3 = nullptr;
	DVKModel* m_Model = nullptr;
	VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;
	VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet0 = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet1 = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet2 = VK_NULL_HANDLE;
	VkDescriptorSet                 m_DescriptorSet3 = VK_NULL_HANDLE;
	ImageGUIContext* m_GUI = nullptr;
};
