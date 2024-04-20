#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class OptimizeRenderTarget :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

public:
	OptimizeRenderTarget(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~OptimizeRenderTarget();
	virtual bool PreInit() override;
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
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
	DVKModel* m_Quad = nullptr;
	DVKRenderTarget* m_RenderTarget = nullptr;
	DVKTexture* m_RTColor = nullptr;
	DVKTexture* m_RTDepth = nullptr;
	ModelViewProjectionBlock    m_MVPData;
	DVKModel* m_ModelScene = nullptr;
	DVKShader* m_SceneShader = nullptr;
	TextureArray                m_SceneDiffuses;
	MaterialArray               m_SceneMaterials;
	MatMeshArray                m_SceneMatMeshes;
	DVKMaterial* m_FilterMaterial;
	DVKShader* m_FilterShader;
	ImageGUIContext* m_GUI = nullptr;
};
