#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class PNTessellationDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};

	struct TessParamBlock
	{
		Vector4 level;
	};
public:
	PNTessellationDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~PNTessellationDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool m_Ready = false;
	DVKModel* m_Model = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKCamera              m_ViewCamera;
	ModelViewProjectionBlock        m_MVPParam;
	TessParamBlock                  m_TessParam;
	ImageGUIContext* m_GUI = nullptr;
};