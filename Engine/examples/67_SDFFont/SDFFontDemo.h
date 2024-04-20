#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class SDFFontDemo :public ModuleBase
{
public:

	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct SDFParamBlock
	{
		Vector4 param0;    // Distance Mark、Outline Distance Mark、Glow Distance Mark、Smooth Delta
		Vector4 param1;    // Shadow Smooth、Glow Smooth、Shadow Offset X、Shadow Offset Y
		Vector4 param2;    // type
		Vector4 mainColor;
		Vector4 outlineColor;
		Vector4 glowColor;
		Vector4 shadowColor;
	};
public:
	SDFFontDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~SDFFontDemo();
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
	DVKCamera          m_ViewCamera;

	ModelViewProjectionBlock    m_MVPData;
	SDFParamBlock               m_SDFData;

	DVKModel* m_Model = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKTexture* m_Texture = nullptr;
	DVKMaterial* m_Material = nullptr;
	ImageGUIContext* m_GUI = nullptr;
};
