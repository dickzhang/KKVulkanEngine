#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define INSTANCE_COUNT 8000
class MSAADemo :public ModuleBase
{
public:

	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};
public:
	MSAADemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~MSAADemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void CreateMSAATexture();
	void DestroyMSAATexture();
	void CreateMSAAFrameBuffers();
	void CreateFrameBuffers() override;
	void CreateMSAARenderPass();
	void CreateRenderPass() override;
	bool UpdateUI(float time,float delta);
	void GenerateLineSphere(std::vector<float>& outVertices,int32 sphslices,float scale);
	VkSampleCountFlagBits GetMaxUsableSampleCount();
	void DestroyMaterials();
	void CreateMaterials();
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();
private:
	bool                        m_Ready = false;
	ImageGUIContext* m_GUI = nullptr;
	DVKCamera          m_ViewCamera;

	ModelViewProjectionBlock    m_MVPData;

	VkSampleCountFlagBits       m_MSAACount = VK_SAMPLE_COUNT_8_BIT;
	DVKTexture* m_MSAAColorTexture = nullptr;
	DVKTexture* m_MSAADepthTexture = nullptr;
	bool                        m_MSAAEnable = false;

	DVKModel* m_LineModel = nullptr;
	DVKShader* m_LineShader = nullptr;
	DVKMaterial* m_MSAAMaterial = nullptr;
	DVKMaterial* m_NoneMaterial = nullptr;
};
