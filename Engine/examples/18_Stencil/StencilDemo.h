#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class StencilDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct RayParamBlock
	{
		Vector3    color;
		float      power;
		Vector3    viewDir;
		float      padding;
	};

public:
	StencilDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~StencilDemo();
	virtual bool PreInit() override;
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateUniform(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	typedef std::vector<DVKTexture*>           TextureArray;
	typedef std::vector<DVKMaterial*>          MaterialArray;
	typedef std::vector<std::vector<DVKMesh*>> MatMeshArray;
	bool                            m_Ready = false;
	DVKCamera              m_ViewCamera;
	ModelViewProjectionBlock        m_MVPData;
	RayParamBlock                   m_RayData;
	DVKShader* m_DiffuseShader = nullptr;
	DVKModel* m_ModelRole = nullptr;
	DVKTexture* m_RoleDiffuse = nullptr;
	DVKMaterial* m_RoleMaterial = nullptr;
	DVKModel* m_ModelScene = nullptr;
	TextureArray                    m_SceneDiffuses;
	MaterialArray                   m_SceneMaterials;
	MatMeshArray                    m_SceneMatMeshes;
	DVKShader* m_RayShader = nullptr;
	DVKMaterial* m_RayMaterial = nullptr;
	ImageGUIContext* m_GUI = nullptr;
};
