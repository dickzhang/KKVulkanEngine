#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define OBJECT_COUNT 1024 * 256
class ComputeFrustumDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};

	struct FrustumParamBlock
	{
		Vector4 count;
		Vector4 frustumPlanes[6];
	};
public:
	ComputeFrustumDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ComputeFrustumDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	bool IsInFrustum(int32 index);
	void RenderSpheres(VkCommandBuffer commandBuffer,DVKCamera& camera);
	void SetupComputeCommand();
	void SetupGfxCommand(int32 backBufferIndex);
	void UpdateFrustumPlanes();
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	typedef std::vector<DVKTexture*>           TextureArray;
	typedef std::vector<DVKMaterial*>          MaterialArray;
	typedef std::vector<std::vector<DVKMesh*>> MatMeshArray;
	bool                            m_Ready = false;
	DVKModel* m_ModelSphere = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKBuffer* m_MatrixBuffer = nullptr;
	DVKBuffer* m_CullingBuffer = nullptr;
	Matrix4x4                       m_ObjModels[OBJECT_COUNT];
	DVKCamera              m_ViewCamera;
	DVKCamera              m_TopCamera;
	FrustumParamBlock               m_FrustumParam;
	DVKShader* m_ComputeShader = nullptr;
	DVKCompute* m_ComputeProcessor = nullptr;
	DVKCommandBuffer* m_ComputeCommand = nullptr;
	ModelViewProjectionBlock        m_MVPParam;
	float                           m_Radius;
	int32                           m_DrawCall = 0;
	bool                            m_UseGPU = true;
	ImageGUIContext* m_GUI = nullptr;
};
