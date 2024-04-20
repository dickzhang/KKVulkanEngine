#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#include "meshoptimizer.h"
#define LOD_GROUP_SIZE 15

class MeshLodDemo :public ModuleBase
{
public:
	struct LodGroup
	{
		uint32 start;
		uint32 count;
	};

	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};
public:
	MeshLodDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~MeshLodDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void ProcessMesh(DVKCommandBuffer* cmdBuffer);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool m_Ready = false;
	DVKModel* m_Model = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKCamera              m_ViewCamera;
	ModelViewProjectionBlock        m_MVPParam;
	LodGroup                        m_LodGroups[LOD_GROUP_SIZE];
	int32                           m_LodIndex = 0;
	DVKVertexBuffer* m_VertexBuffer = nullptr;
	DVKIndexBuffer* m_IndexBuffer = nullptr;
	ImageGUIContext* m_GUI = nullptr;
};
