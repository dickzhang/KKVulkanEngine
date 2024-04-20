#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class SimpleTessellationDemo :public ModuleBase
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
		Vector4 levelOuter;
		Vector4 levelInner;
	};
public:
	SimpleTessellationDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~SimpleTessellationDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void DrawModel(VkCommandBuffer commandBuffer,DVKModel* model,DVKMaterial* material);
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool m_Ready = false;
	DVKModel* m_PatchTriangle = nullptr;
	DVKModel* m_PatchQuat = nullptr;
	DVKModel* m_PatchIso = nullptr;
	DVKShader* m_ShaderTri = nullptr;
	DVKShader* m_ShaderQuad = nullptr;
	DVKShader* m_ShaderIso = nullptr;
	DVKMaterial* m_MaterialTri = nullptr;
	DVKMaterial* m_MaterialQuat = nullptr;
	DVKMaterial* m_MaterialIso = nullptr;
	DVKCamera              m_ViewCamera;
	ModelViewProjectionBlock        m_MVPParam;
	TessParamBlock                  m_TessParam;
	ImageGUIContext* m_GUI = nullptr;
};
