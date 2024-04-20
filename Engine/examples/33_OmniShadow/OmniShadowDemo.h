#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class OmniShadowDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct LightCameraParamBlock
	{
		Matrix4x4 model;
		Matrix4x4 view[6];
		Matrix4x4 projection;
		Vector4 position;
	};

	struct ShadowParamBlock
	{
		Vector4 position;
		Vector4 bias;
	};
public:
	OmniShadowDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~OmniShadowDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void CreateRenderTarget();
    void DestroyRenderTarget();
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

    bool                        m_Ready = false;
    DVKCamera          m_ViewCamera;

    // Shadow Rendertarget
    DVKRenderTarget* m_ShadowRTT = nullptr;
    DVKTexture* m_RTDepth = nullptr;
    DVKTexture* m_RTColor = nullptr;

    // depth
    DVKShader* m_DepthShader = nullptr;
    DVKMaterial* m_DepthMaterial = nullptr;

    // mvp
    ModelViewProjectionBlock    m_MVPData;
    DVKModel* m_ModelScene = nullptr;

    // light
    LightCameraParamBlock       m_LightCamera;
    ShadowParamBlock            m_ShadowParam;

    // obj render
    DVKShader* m_SimpleShadowShader = nullptr;
    DVKMaterial* m_SimpleShadowMaterial = nullptr;

    DVKShader* m_PCFShadowShader = nullptr;
    DVKMaterial* m_PCFShadowMaterial = nullptr;

    Vector4                     m_LightPosition;

    int32                       m_Selected = 1;
    std::vector<const char*>    m_ShadowNames;
    MaterialArray               m_ShadowList;

    ImageGUIContext* m_GUI = nullptr;
};
