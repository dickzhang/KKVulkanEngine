#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class SimpleShadowDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct DirectionalLightBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
		Vector4 direction;
	};
public:
	SimpleShadowDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~SimpleShadowDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateLight(float time,float delta);
	bool UpdateUI(float time,float delta);
	void CreateRenderTarget();
	void DestroyRenderTarget();
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	typedef std::vector<DVKTexture*>           TextureArray;
	typedef std::vector<DVKMaterial*>          MaterialArray;
	typedef std::vector<std::vector<DVKMesh*>> MatMeshArray;

	bool                        m_Ready = false;
	DVKCamera          m_ViewCamera;

	// Debug
	DVKModel* m_Quad = nullptr;
	DVKMaterial* m_DebugMaterial;
	DVKShader* m_DebugShader;

	// Shadow Rendertarget
	DVKRenderTarget* m_ShadowRTT = nullptr;
	DVKTexture* m_ShadowMap = nullptr;

	// depth
	DVKShader* m_DepthShader = nullptr;
	DVKMaterial* m_DepthMaterial = nullptr;

	// mvp
	ModelViewProjectionBlock    m_MVPData;
	DVKModel* m_ModelScene = nullptr;

	// light
	DirectionalLightBlock       m_LightCamera;

	// obj render
	DVKShader* m_ShadeShader = nullptr;
	DVKMaterial* m_ShadeMaterial = nullptr;

	bool                        m_AnimLight = true;

	ImageGUIContext* m_GUI = nullptr;
};
