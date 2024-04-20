#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class PBRIBLDemo :public ModuleBase
{
public:
    struct ModelViewProjectionBlock
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 proj;
    };

    struct PBRParamBlock
    {
        Vector4 param;
        Vector4 cameraPos;
        Vector4 lightColor;
        Vector4 envParam;
    };
public:
	PBRIBLDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~PBRIBLDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void LoadEnvAssets();
    void LoadModelAssets();
    void GenEnvPrefiltered();
    void GenEnvIrradiance();
    void GenEnvBRDFLut();
    void DestroyAssets();
    void SetupCommandBuffers(int32 backBufferIndex);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    bool m_Ready = false;
    DVKModel* m_EnvModel = nullptr;
    DVKTexture* m_EnvTexture = nullptr;
    DVKShader* m_EnvShader = nullptr;
    DVKMaterial* m_EnvMaterial = nullptr;
    DVKTexture* m_EnvIrradiance = nullptr;
    DVKTexture* m_EnvBRDFLut = nullptr;
    DVKTexture* m_EnvPrefiltered = nullptr;
    DVKModel* m_Model = nullptr;
    DVKShader* m_Shader = nullptr;
    DVKMaterial* m_Material = nullptr;
    DVKTexture* m_TexAlbedo = nullptr;
    DVKTexture* m_TexNormal = nullptr;
    DVKTexture* m_TexORMParam = nullptr;
    DVKCamera          m_ViewCamera;
    ModelViewProjectionBlock    m_MVPParam;
    PBRParamBlock               m_PBRParam;
    ImageGUIContext* m_GUI = nullptr;
};
