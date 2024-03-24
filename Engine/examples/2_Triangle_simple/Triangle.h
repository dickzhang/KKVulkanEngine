#pragma once

#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/ModuleBase.h"
#include "Graphics/DVKBuffer.h"
#include "Graphics/DVKCommand.h"
#include "Graphics/DVKUtils.h"
#include "Graphics/DVKCamera.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include <vector>

class TriangleModule : public ModuleBase
{
public:
    struct Vertex
    {
        float position[3];
        float color[3];
    };

    struct UBOData
    {
        Matrix4x4 model;
        Matrix4x4 view;
        Matrix4x4 projection;
    };
public:
    TriangleModule(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);

    virtual ~TriangleModule();

    virtual bool PreInit() override;

    virtual bool Init() override;

    virtual void Exist() override;

    virtual void Loop(float time,float delta) override;

private:

    void Draw(float time,float delta);

    void SetupCommandBuffers();

    void CreateDescriptorSet();

    void CreateDescriptorPool();

    void DestroyDescriptorPool();

    void CreatePipelines();

    void DestroyPipelines();

    void CreateDescriptorSetLayout();

    void DestroyDescriptorSetLayout();

    void UpdateUniformBuffers(float time,float delta);

    void CreateUniformBuffers();

    void DestroyUniformBuffers();

    void CreateMeshBuffers();

    void DestroyMeshBuffers();

private:
    bool                            m_Ready = false;
    DVKCamera              m_ViewCamera;

    UBOData                         m_MVPData;

    DVKBuffer* m_IndexBuffer = nullptr;
    DVKBuffer* m_VertexBuffer = nullptr;
    DVKBuffer* m_MVPBuffer = nullptr;

    VkDescriptorSetLayout           m_DescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet                 m_DescriptorSet = VK_NULL_HANDLE;
    VkPipelineLayout                m_PipelineLayout = VK_NULL_HANDLE;
    VkDescriptorPool                m_DescriptorPool = VK_NULL_HANDLE;

    VkPipeline                      m_Pipeline = VK_NULL_HANDLE;

    uint32                          m_IndicesCount = 0;
};

