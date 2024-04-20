#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

#define LIGHT_SIZE 512
#define TILE_SIZE 16
#define LIGHT_SIZE_PER_TILE 63

class TileBasedForwardRenderingDemo :public ModuleBase
{
public:
	struct PointLight
	{
		Vector3 position;
		float radius = 2.5f;

		Vector3 color = Vector3(1.0f,1.0f,1.0f);
		float padding;

		PointLight()
		{

		}

		PointLight(const Vector3& inPosition,float inRadius,const Vector3& inColor)
			: position(inPosition)
			,radius(inRadius)
			,color(inColor)
		{

		}
	};

	struct LightsParamBlock
	{
		Vector4 count = Vector4(0,0,0,0);
		PointLight lights[LIGHT_SIZE];

		LightsParamBlock()
		{

		}
	};

	struct LightsInfo
	{
		Vector3 position[LIGHT_SIZE];
		Vector3 direction[LIGHT_SIZE];
	};

	struct LightVisiblity
	{
		uint32 count;
		uint32 lightindices[LIGHT_SIZE_PER_TILE];
	};

	struct CullingParamBlock
	{
		Matrix4x4 invViewProj;
		Vector4 frameSize;
		Vector4 tileNum;
		Vector4 pos;
	};

	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};
public:
	TileBasedForwardRenderingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~TileBasedForwardRenderingDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void UpdateLights(float time,float delta);
	void InitLights();
	void LoadAssets();
	void DestroyAssets();
	void PreDepthPass(int backBufferIndex);
	void FinnalPass(int backBufferIndex);
	void SetupComputeCommand(int32 backBufferIndex);
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();
private:
	bool                        m_Ready = false;

	// scene
	DVKModel* m_Model = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKMaterial* m_Material = nullptr;

	// pre-depth rt
	DVKShader* m_DepthShader = nullptr;
	DVKMaterial* m_DepthMaterial = nullptr;
	DVKTexture* m_PreDepthTexture = nullptr;
	DVKRenderTarget* m_PreDepthRTT = nullptr;

	// compute shader
	DVKShader* m_ComputeShader = nullptr;
	DVKCompute* m_ComputeProcessor = nullptr;
	DVKBuffer* m_LightsCullingBuffer = nullptr;
	DVKCamera          m_ViewCamera;
	Vector4                     m_Debug;
	LightsInfo                  m_LightInfo;
	LightsParamBlock            m_LightParam;
	CullingParamBlock           m_CullingParam;
	ModelViewProjectionBlock    m_MVPParam;
	int32                       m_TileCountPerRow = 0;
	int32                       m_TileCountPerCol = 0;
	ImageGUIContext* m_GUI = nullptr;
};
