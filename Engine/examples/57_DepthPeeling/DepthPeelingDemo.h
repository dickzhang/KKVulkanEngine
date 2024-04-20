#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define MESH_SIZE 11
class DepthPeelingDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};
public:
	DepthPeelingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~DepthPeelingDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void CreateRendertarget();
	void LoadAssets();
	void DestroyAssets();
	void OpaquePass(VkCommandBuffer commandBuffer);
	void RenderFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex);
	void PeelPass(VkCommandBuffer commandBuffer,DVKMaterial* material,DVKRenderTarget* renderTarget,int32 layer);
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                        m_Ready = false;
	DVKModel* m_Quad = nullptr;
	// scene
	DVKModel* m_Model = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKMaterial* m_Material = nullptr;

	// source
	DVKTexture* m_TexSourceColor = nullptr;
	DVKTexture* m_TexSourceDepth = nullptr;
	DVKRenderTarget* m_RTSource = nullptr;

	//
	DVKModel* m_SuzanneModel = nullptr;
	DVKShader* m_PeelShader = nullptr;
	DVKMaterial* m_PeelMaterials[5];

	// peel
	DVKTexture* m_TexDepth0 = nullptr;
	DVKTexture* m_TexDepth1 = nullptr;

	DVKTexture* m_TexPeel0 = nullptr;
	DVKRenderTarget* m_RTPeel0 = nullptr;

	DVKTexture* m_TexPeel1 = nullptr;
	DVKRenderTarget* m_RTPeel1 = nullptr;

	DVKTexture* m_TexPeel2 = nullptr;
	DVKRenderTarget* m_RTPeel2 = nullptr;

	DVKTexture* m_TexPeel3 = nullptr;
	DVKRenderTarget* m_RTPeel3 = nullptr;

	DVKTexture* m_TexPeel4 = nullptr;
	DVKRenderTarget* m_RTPeel4 = nullptr;

	// finnal pass
	DVKShader* m_CombineShader = nullptr;
	DVKMaterial* m_CombineMaterial = nullptr;

	DVKCamera          m_ViewCamera;

	ModelViewProjectionBlock    m_MVPParam;
	Vector4                     m_PeelParam;

	ImageGUIContext* m_GUI = nullptr;
};
