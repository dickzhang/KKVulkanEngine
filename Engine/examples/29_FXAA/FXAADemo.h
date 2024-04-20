#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

enum FXAATypes
{
	Normal = 0,
	Default,
	Fast,
	High,
	Best,
	Count,
};
class FXAADemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct FXAAParamBlock
	{
		Vector2 frame;
		Vector2 padding;
	};
public:
	FXAADemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~FXAADemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void GenerateLineSphere(std::vector<float>& outVertices,int32 sphslices,float scale);
	void DestroyMaterials();
	void CreateMaterials();
	void CreateRenderTarget();
	void DestroyRenderTarget();
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                        m_Ready = false;
	DVKCamera          m_ViewCamera;
	ImageGUIContext* m_GUI = nullptr;

	ModelViewProjectionBlock    m_MVPData;
	FXAAParamBlock              m_FXAAParam;

	DVKModel* m_LineModel = nullptr;
	DVKShader* m_LineShader = nullptr;
	DVKMaterial* m_LineMaterial = nullptr;

	DVKMaterial* m_NormalMaterial = nullptr;
	DVKShader* m_NormalShader = nullptr;

	DVKMaterial* m_FXAADefaultMaterial = nullptr;
	DVKShader* m_FXAADefaultShader = nullptr;

	DVKMaterial* m_FXAAFastMaterial = nullptr;
	DVKShader* m_FXAAFastShader = nullptr;

	DVKMaterial* m_FXAAHighMaterial = nullptr;
	DVKShader* m_FXAAHighShader = nullptr;

	DVKMaterial* m_FXAABestMaterial = nullptr;
	DVKShader* m_FXAABestShader = nullptr;

	DVKRenderTarget* m_RenderTarget = nullptr;
	DVKTexture* m_RTColor = nullptr;
	DVKTexture* m_RTDepth = nullptr;
	DVKModel* m_Quad = nullptr;

	FXAATypes                           m_Select = FXAATypes::Normal;
	std::vector<DVKMaterial*>  m_FilterMaterials;
	std::vector<const char*>            m_FilterNames;
	bool                                m_AutoRotate = false;
};
