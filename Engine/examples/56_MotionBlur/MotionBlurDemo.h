#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define MESH_SIZE 11
class MotionBlurDemo :public ModuleBase
{
public:
    struct ModelViewProjectionBlock
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 proj;
    };
public:
	MotionBlurDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~MotionBlurDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void CreateSourceRT();
    void LoadAssets();
    void DestroyAssets();
    void VelocityPass(VkCommandBuffer commandBuffer);
    void RenderFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex);
    void SetupCommandBuffers(int32 backBufferIndex);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    bool                        m_Ready = false;
    DVKModel* m_Quad = nullptr;
    // scene
    DVKModel* m_Model = nullptr;
    DVKShader* m_Shader = nullptr;
    DVKTexture* m_Textures[MESH_SIZE];
    DVKMaterial* m_Materials[MESH_SIZE];

    // Velocity
    ModelViewProjectionBlock    m_PreviousMVP[MESH_SIZE];

    // source
    DVKTexture* m_TexSourceColor = nullptr;
    DVKTexture* m_TexSourceVelocity = nullptr;
    DVKTexture* m_TexSourceDepth = nullptr;
    DVKRenderTarget* m_RTSource = nullptr;

    // finnal pass
    DVKShader* m_CombineShader = nullptr;
    DVKMaterial* m_CombineMaterial = nullptr;
    DVKCamera          m_ViewCamera;
    ModelViewProjectionBlock    m_MVPParam;
    float                       m_Speed = 1.2f;
    bool                        m_Enable = true;
    Vector4                     m_DebugParam;
    ImageGUIContext* m_GUI = nullptr;
};
