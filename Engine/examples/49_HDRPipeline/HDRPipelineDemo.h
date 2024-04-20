#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class HDRPipelineDemo :public ModuleBase
{
public:
    struct ModelViewProjectionBlock
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 proj;
    };

    struct ParamBlock
    {
        Vector4 intensity;
    };
public:
	HDRPipelineDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~HDRPipelineDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void CreateLuminanceRT();
    void CreateBlurRT();
    void CreateBrightRT();
    void CreateSourceRT();
    void LoadAssets();
    void DestroyAssets();
    void RenderScene(VkCommandBuffer commandBuffer);
    void SourcePass(VkCommandBuffer commandBuffer);
    void BrightPass(VkCommandBuffer commandBuffer);
    void BlurHPass(VkCommandBuffer commandBuffer);
    void LuminancePass(VkCommandBuffer commandBuffer);
    void BlurVPass(VkCommandBuffer commandBuffer);
    void RenderPipeline(VkCommandBuffer commandBuffer);
    void RenderFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex);
    void SetupCommandBuffers(int32 backBufferIndex);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    bool                        m_Ready = false;
    DVKModel* m_Quad = nullptr;
    DVKShader* m_DebugShader;
    DVKMaterial* m_DebugBright;
    DVKMaterial* m_DebugBlurH;
    DVKMaterial* m_DebugBlurV;
    DVKMaterial* m_DebugLumDownsample;
    DVKMaterial* m_DebugLum1x1;
    DVKMaterial* m_DebugLum3x3;
    DVKMaterial* m_DebugLum9x9;
    DVKMaterial* m_DebugLum27x27;
    DVKMaterial* m_DebugLum81x81;
    DVKMaterial* m_DebugLum243x243;
    // scene
    DVKModel* m_SceneModel = nullptr;
    DVKShader* m_SceneShader = nullptr;
    DVKTexture* m_SceneTextures[2];
    DVKMaterial* m_SceneMaterials[2];

    // source
    DVKTexture* m_TexSourceColor = nullptr;
    DVKTexture* m_TexSourceDepth = nullptr;
    DVKRenderTarget* m_RTSource = nullptr;

    // bright pass
    DVKTexture* m_TexBright = nullptr;
    DVKRenderTarget* m_RTBright = nullptr;
    DVKShader* m_BrightShader = nullptr;
    DVKMaterial* m_BrightMaterial = nullptr;

    // blur h pass
    DVKTexture* m_TexBlurH = nullptr;
    DVKRenderTarget* m_RTBlurH = nullptr;
    DVKShader* m_BlurHShader = nullptr;
    DVKMaterial* m_BlurHMaterial = nullptr;

    // blur v pass
    DVKTexture* m_TexBlurV = nullptr;
    DVKRenderTarget* m_RTBlurV = nullptr;
    DVKShader* m_BlurVShader = nullptr;
    DVKMaterial* m_BlurVMaterial = nullptr;

    // luminance
    DVKTexture* m_TexLuminances[7];
    DVKRenderTarget* m_RTLuminances[7];
    DVKShader* m_LuminanceDowmSampleShader = nullptr;
    DVKShader* m_LuminanceShader = nullptr;
    DVKMaterial* m_LuminanceMaterials[7];

    // finnal pass
    DVKShader* m_FinalShader = nullptr;
    DVKMaterial* m_FinalMaterial = nullptr;
    DVKCamera          m_ViewCamera;
    ModelViewProjectionBlock    m_MVPParam;
    ParamBlock                  m_ParamData;
    bool                        m_Debug = false;
    ImageGUIContext* m_GUI = nullptr;
};
