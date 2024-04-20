#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class GodRayDemo :public ModuleBase
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
        Vector4 param;
        Vector4 color;
    };
public:
	GodRayDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~GodRayDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void CreateSunRT();
    void CreateSourceRT();
    void LoadModelAssets();
    void DestroyAssets();
    void DrawScene(VkCommandBuffer commandBuffer);
    void DrawFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex);
    void DrawLight(VkCommandBuffer commandBuffer);
    void SetupCommandBuffers(int32 backBufferIndex);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    bool                        m_Ready = false;
    // object
    DVKModel* m_Model = nullptr;
    DVKShader* m_Shader = nullptr;
    DVKMaterial* m_Material = nullptr;
    DVKTexture* m_TexAlbedo = nullptr;
    DVKTexture* m_TexNormal = nullptr;

    // sphere
    DVKModel* m_SphereModel = nullptr;
    // emissive
    DVKShader* m_EmissiveShader = nullptr;
    DVKMaterial* m_EmissiveMaterial = nullptr;

    // combine
    DVKShader* m_CombineShader = nullptr;
    DVKMaterial* m_CombineMaterial = nullptr;

    // source
    DVKTexture* m_TexSourceColor = nullptr;
    DVKTexture* m_TexSourceDepth = nullptr;
    DVKRenderTarget* m_RTSource = nullptr;

    // light source
    DVKTexture* m_TexLight = nullptr;
    DVKRenderTarget* m_RTLight = nullptr;

    DVKCamera          m_ViewCamera;

    ParamBlock                  m_CombineParam;
    ModelViewProjectionBlock    m_MVPParam;
    ImageGUIContext* m_GUI = nullptr;
};
