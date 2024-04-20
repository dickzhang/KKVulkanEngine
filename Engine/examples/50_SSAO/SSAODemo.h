#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define TEX_SIZE 4
class SSAODemo :public ModuleBase
{
public:
    struct ModelViewProjectionBlock
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 proj;
    };

    struct DebugParamBlock
    {
        Vector4 data;
    };
public:
	SSAODemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~SSAODemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void CreateSourceRT();
    void LoadSceneRes(DVKCommandBuffer* cmdBuffer);
    void LoadPreDepthRes(DVKCommandBuffer* cmdBuffer);
    void LoadComputeAoRes(DVKCommandBuffer* cmdBuffer);
    void LoadCombineRes(DVKCommandBuffer* cmdBuffer);
    void LoadBlurAndUpsampleRes(DVKCommandBuffer* cmdBuffer);
    void LoadAssets();
    void DestroyAssets();
    void ScenePass(VkCommandBuffer commandBuffer);
    void CombinePass(VkCommandBuffer commandBuffer,int32 backBufferIndex);
    void SetupCommandBuffers(int32 backBufferIndex);
    void BlurAndUpsamplePass(VkCommandBuffer commandBuffer);
    void ComputeAoPass(VkCommandBuffer commandBuffer);
    void PrepareDepthPass(VkCommandBuffer commandBuffer);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    bool                        m_Ready = false;
    float                       m_SampleThickness[12];

    DVKModel* m_Quad = nullptr;

    // scene
    DVKModel* m_SceneModel = nullptr;
    DVKShader* m_SceneShader = nullptr;
    DVKTexture* m_SceneTextures[TEX_SIZE];
    DVKMaterial* m_SceneMaterials[TEX_SIZE];

    // source
    DVKTexture* m_TexSourceColor = nullptr;
    DVKTexture* m_TexSourceLinearDepth = nullptr;
    DVKTexture* m_TexSourceDepth = nullptr;
    DVKRenderTarget* m_RTSource = nullptr;

    // prepare depth
    DVKTexture* m_TexLinearDepth = nullptr;
    DVKTexture* m_TexDepthDownSize = nullptr;
    DVKTexture* m_TexDepthTiled = nullptr;
    DVKShader* m_ShaderDepthPrepare = nullptr;
    DVKCompute* m_ComputeDepthPrepare = nullptr;

    // ao merge
    DVKShader* m_ShaderAoMerge = nullptr;
    DVKCompute* m_ComputeAoMerge = nullptr;
    DVKTexture* m_TexAoMerge = nullptr;

    // upsample and blue
    DVKShader* m_ShaderBlurAndUpsample = nullptr;
    DVKCompute* m_ComputeBlurAndUpsample = nullptr;
    DVKTexture* m_TexAoFullScreen = nullptr;

    // finnal pass
    DVKShader* m_CombineShader = nullptr;
    DVKMaterial* m_CombineMaterial = nullptr;

    DVKCamera          m_ViewCamera;

    ModelViewProjectionBlock    m_MVPParam;
    DebugParamBlock             m_DebugParam;

    float                       m_BlurTolerance = -5.0f;
    float                       m_UpsampleTolerance = -7.0f;
    float                       m_NoiseFilterTolerance = -3.0f;
    float                       m_ScreenspaceDiameter = 10.0f;
    float                       m_RejectionFalloff = 2.5f;
    float                       m_Accentuation = 0.10f;

    ImageGUIContext* m_GUI = nullptr;
};
