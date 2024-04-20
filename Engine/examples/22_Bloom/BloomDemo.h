#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class BloomDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct FilterParamBlock
	{
		float width;
		float height;
		float step;
		float bright;
	};
public:
	BloomDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~BloomDemo();
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

	DVKRenderTarget* m_RTTNormal = nullptr;
	DVKRenderTarget* m_RTTQuater0 = nullptr;
	DVKRenderTarget* m_RTTQuater1 = nullptr;

	DVKTexture* m_RTColor = nullptr;
	DVKTexture* m_RTDepth = nullptr;
	DVKTexture* m_RTColorQuater0 = nullptr;
	DVKTexture* m_RTColorQuater1 = nullptr;

	ModelViewProjectionBlock    m_MVPData;

	DVKModel* m_ModelScene = nullptr;
	DVKShader* m_SceneShader = nullptr;
	TextureArray                m_SceneDiffuses;
	MaterialArray               m_SceneMaterials;
	MatMeshArray                m_SceneMatMeshes;

	FilterParamBlock            m_FilterParam;

	DVKMaterial* m_CombineMaterial;
	DVKShader* m_CombineShader;

	DVKMaterial* m_BrightMaterial = nullptr;
	DVKShader* m_BrightShader = nullptr;

	DVKMaterial* m_BlurHMaterial = nullptr;
	DVKShader* m_BlurHShader = nullptr;

	DVKMaterial* m_BlurVMateria = nullptr;
	DVKShader* m_BlurVShader = nullptr;

	ImageGUIContext* m_GUI = nullptr;
};
