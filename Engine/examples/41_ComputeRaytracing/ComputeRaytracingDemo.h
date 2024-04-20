#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class ComputeRaytracingDemo :public ModuleBase
{
public:
	struct Material
	{
		Vector4 ambientColor;
		Vector4 diffuseColor;
		Vector4 specularColor;
		Vector4 reflectedColor;
		Vector4 refractedColor;
		Vector4 refractiveIndex;
	};

	struct RaytracingParamBlock
	{
		Vector4     lightPos;
		Vector4     cameraPos;
		Matrix4x4   invProjection;
		Matrix4x4   invView;
		Material    materials[10];
	};
public:
	ComputeRaytracingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ComputeRaytracingDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupComputeCommand();
	void SetupGfxCommand(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                            m_Ready = false;

	DVKBuffer* m_SceneBuffer = nullptr;
	DVKModel* m_SceneModel = nullptr;

	DVKModel* m_Quad = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;

	DVKTexture* m_ComputeTarget = nullptr;
	DVKShader* m_ComputeShader = nullptr;
	DVKCompute* m_ComputeProcessor = nullptr;
	DVKCommandBuffer* m_ComputeCommand = nullptr;

	RaytracingParamBlock            m_RaytracingParam;

	ImageGUIContext* m_GUI = nullptr;
};
