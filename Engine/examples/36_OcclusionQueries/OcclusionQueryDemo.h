#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define OBJECT_COUNT 1024
class OcclusionQueryDemo :public ModuleBase
{
public:
    struct ModelViewProjectionBlock
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 proj;
    };
public:
	OcclusionQueryDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~OcclusionQueryDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
    void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void LoadAssets();
    void DestroyAssets();
    void RenderOcclusions(VkCommandBuffer commandBuffer,DVKCamera& camera);
    void RenderGround(VkCommandBuffer commandBuffer,DVKCamera& camera);
    void RenderSpheres(VkCommandBuffer commandBuffer,DVKCamera& camera);
    void BeginMainPass(VkCommandBuffer commandBuffer,int32 backBufferIndex);
    void SetupCommandBuffers(int32 backBufferIndex);
    void InitParmas();
    void CreateGUI();
    void DestroyGUI();

private:
    typedef std::vector<DVKTexture*>           TextureArray;
    typedef std::vector<DVKMaterial*>          MaterialArray;
    typedef std::vector<std::vector<DVKMesh*>> MatMeshArray;
    bool                        m_Ready = false;
    VkQueryPool                 m_QueryPool;
    DVKModel* m_ModelSphere = nullptr;
    DVKModel* m_ModelGround = nullptr;
    DVKMaterial* m_Material = nullptr;
    DVKShader* m_Shader = nullptr;
    DVKMaterial* m_SimpleMaterial = nullptr;
    DVKShader* m_SimpleShader = nullptr;
    Matrix4x4                   m_ObjModels[OBJECT_COUNT];
    uint64                      m_QuerySamples[OBJECT_COUNT];
    Vector3                     m_SphereCenter;
    float                       m_SphereRadius;
    bool                        m_EnableQuery = true;
    DVKCamera          m_ViewCamera;
    DVKCamera          m_TopCamera;
    ModelViewProjectionBlock    m_MVPParam;
    ImageGUIContext* m_GUI = nullptr;
};
