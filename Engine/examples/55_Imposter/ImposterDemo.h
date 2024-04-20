#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class ImposterDemo :public ModuleBase
{
public:
    struct ModelViewProjectionBlock
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 proj;
    };
public:
	ImposterDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ImposterDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void GenImposterAssets();
    void LoadModelAssets();
    void DestroyAssets();
    void SetupCommandBuffers(int32 backBufferIndex);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    bool                        m_Ready = false;

    // combine
    DVKShader* m_ImposterShader = nullptr;
    DVKMaterial* m_ImposterMaterial = nullptr;

    // imposter diffuse + normal
    DVKTexture* m_ImposterDiffuse = nullptr;
    DVKTexture* m_ImposterNormal = nullptr;

    int32                       m_TileCount = 16;

    DVKCamera          m_ViewCamera;
    ModelViewProjectionBlock    m_MVPParam;

    ImageGUIContext* m_GUI = nullptr;
};
