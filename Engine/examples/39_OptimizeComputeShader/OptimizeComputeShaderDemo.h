#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class OptimizeComputeShaderDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};
public:
	OptimizeComputeShaderDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~OptimizeComputeShaderDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void ProcessImage();
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                            m_Ready = false;
	DVKModel* m_ModelPlane = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKTexture* m_Texture = nullptr;

	DVKCamera              m_ViewCamera;
	ModelViewProjectionBlock        m_MVPParam;

	DVKTexture* m_ComputeTargets[3];
	DVKShader* m_ComputeShaders[3];
	DVKCompute* m_ComputeProcessors[3];

	std::vector<const char*>        m_FilterNames;
	int32                           m_FilterIndex;

	ImageGUIContext* m_GUI = nullptr;
};
