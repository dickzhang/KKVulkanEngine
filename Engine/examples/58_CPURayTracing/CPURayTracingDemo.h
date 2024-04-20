#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"

#include "TaskThread.h"
#include "ThreadTask.h"
#include "TaskThreadPool.h"
#include "RayTracing.h"

#include <vector>
#include <thread>

#define WIDTH 1400
#define HEIGHT 900
#define EPSILON 0.0001
class CPURayTracingDemo :public ModuleBase
{
public:
	CPURayTracingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~CPURayTracingDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	uint8 ToUint8(float f);
	Vector4 ToGammaSpace(Vector4 color);
	void CPURayTracing();
	void LoadAssets();
	void DestroyAssets();
	void SetupGfxCommand(int32 backBufferIndex);
	void InitParmas();

private:
	bool                            m_Ready = false;
	DVKModel* m_SceneModel = nullptr;
	DVKTexture* m_Texture = nullptr;
	DVKModel* m_Quad = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;
};
