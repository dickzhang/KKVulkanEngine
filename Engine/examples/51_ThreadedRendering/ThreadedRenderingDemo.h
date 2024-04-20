#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#define INSTANCE_COUNT 512

struct InstanceData
{
    Matrix4x4   transforms[INSTANCE_COUNT];
    Vector4     colors[INSTANCE_COUNT];
};

struct ParticleData
{
    Vector3 position;
    Vector3 velocity;
    Vector3 direction;
    float   grivity;
    float   time;
    float   lifeTime;
};

struct ModelViewProjectionBlock
{
    Matrix4x4 model;
    Matrix4x4 view;
    Matrix4x4 proj;
};

std::mutex writeMutex;

class ParticleModel
{
public:
    ParticleModel(DVKModel* model,DVKMaterial* material,DVKModel* templat,int32 baseIndex,int32 count)
        : m_Model(model)
        ,m_Material(material)
        ,m_Template(templat)
        ,m_BaseIndex(baseIndex)
        ,m_Count(count)
        ,m_UpdateIndex(0)
    {

    }

    void Draw(VkCommandBuffer commandBuffer,DVKCamera& camera)
    {
        DVKPrimitive* primitive = m_Model->meshes[0]->primitives[0];

        vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());

        m_MVPParam.model = m_Model->meshes[0]->linkNode->GetGlobalMatrix();
        m_MVPParam.view = camera.GetView();
        m_MVPParam.proj = camera.GetProjection();

        {
            std::lock_guard<std::mutex> lockGuard(writeMutex);

            m_Material->BeginFrame();

            m_Material->BeginObject();
            m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
            m_Material->SetLocalUniform("uboTransform",&m_InstanceData,sizeof(InstanceData));
            m_Material->EndObject();

            m_Material->EndFrame();

            m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
        }

        vkCmdBindVertexBuffers(commandBuffer,0,1,&(primitive->vertexBuffer->dvkBuffer->buffer),&(primitive->vertexBuffer->offset));
        vkCmdBindVertexBuffers(commandBuffer,1,1,&(primitive->instanceBuffer->dvkBuffer->buffer),&(primitive->instanceBuffer->offset));
        vkCmdBindIndexBuffer(commandBuffer,primitive->indexBuffer->dvkBuffer->buffer,0,primitive->indexBuffer->indexType);
        vkCmdDrawIndexed(commandBuffer,primitive->indexBuffer->indexCount,m_UpdateIndex,0,0,0);

    }

    void Update(std::vector<Matrix4x4>& bonesData,DVKCamera& camera,float time,float delta)
    {
        // ring buffer
        if(m_UpdateIndex+m_Count>m_Model->meshes[0]->primitives[0]->indexBuffer->instanceCount)
        {
            m_UpdateIndex = 0;
        }

        // move particle
        for(int32 index = 0; index<m_UpdateIndex; ++index)
        {
            if(m_ParticleDatas[index].time>=m_ParticleDatas[index].lifeTime)
            {
                m_InstanceData.colors[index].w = 0;
                continue;
            }

            m_ParticleDatas[index].time += delta;
            m_ParticleDatas[index].position += m_ParticleDatas[index].direction*m_ParticleDatas[index].velocity*delta;
            m_ParticleDatas[index].position.y += m_ParticleDatas[index].grivity*delta;

            Matrix4x4 matrix;
            matrix.SetPosition(m_ParticleDatas[index].position);
            matrix.LookAt(camera.GetTransform().GetOrigin());

            m_InstanceData.colors[index].w = m_ParticleDatas[index].time/m_ParticleDatas[index].lifeTime;
            m_InstanceData.transforms[index] = matrix;
        }

        // init particle
        DVKPrimitive* primitive = m_Template->meshes[0]->primitives[0];
        int32 stride = primitive->vertices.size()/primitive->vertexCount;
        int32 vertBegin = m_BaseIndex*stride;
        int32 vertEnd = (m_BaseIndex+m_Count)*stride;
        int32 objIndex = m_UpdateIndex;

        for(int32 index = vertBegin; index<vertEnd; index += stride)
        {
            Vector3 position(
                primitive->vertices[index+0],
                primitive->vertices[index+1],
                primitive->vertices[index+2]
            );
            IntVector4 skinIndices(
                primitive->vertices[index+6],
                primitive->vertices[index+7],
                primitive->vertices[index+8],
                primitive->vertices[index+9]
            );
            Vector4 skinWeights(
                primitive->vertices[index+10],
                primitive->vertices[index+11],
                primitive->vertices[index+12],
                primitive->vertices[index+13]
            );

            Vector3 finalPos =
                bonesData[skinIndices.x].TransformPosition(position)*skinWeights.x+
                bonesData[skinIndices.y].TransformPosition(position)*skinWeights.y+
                bonesData[skinIndices.z].TransformPosition(position)*skinWeights.z+
                bonesData[skinIndices.w].TransformPosition(position)*skinWeights.w;

            Matrix4x4 matrix;
            matrix.SetPosition(finalPos);
            matrix.LookAt(camera.GetTransform().GetOrigin());

            m_InstanceData.colors[objIndex] = Vector4(MMath::FRandRange(0,1.0f),MMath::FRandRange(0,1.0f),MMath::FRandRange(0,1.0f),1.0f);
            m_InstanceData.transforms[objIndex] = matrix;

            m_ParticleDatas[objIndex].position = finalPos;
            m_ParticleDatas[objIndex].direction = Vector3(MMath::FRandRange(0,1.0f),MMath::FRandRange(0,1.0f),MMath::FRandRange(0,1.0f)).GetSafeNormal();
            m_ParticleDatas[objIndex].velocity = Vector3(MMath::FRandRange(0,1.0f),MMath::FRandRange(0,1.0f),MMath::FRandRange(0,1.0f)).GetSafeNormal()*MMath::FRandRange(5.0f,15.0f);
            m_ParticleDatas[objIndex].grivity = MMath::FRandRange(0,-5.0f);
            m_ParticleDatas[objIndex].lifeTime = MMath::FRandRange(0.25f,0.50f);
            m_ParticleDatas[objIndex].time = 0;

            objIndex += 1;
        }

        m_UpdateIndex += m_Count;
    }

private:

    DVKModel* m_Template;
    DVKModel* m_Model;
    DVKMaterial* m_Material;
    int32                       m_BaseIndex;
    int32                       m_Count;
    int32                       m_UpdateIndex;
    InstanceData                m_InstanceData;
    ParticleData                m_ParticleDatas[INSTANCE_COUNT];
    ModelViewProjectionBlock    m_MVPParam;
};

struct ThreadData
{
    int32 index;
    int32 frameID;
    VkCommandPool commandPool;
    std::vector<ParticleModel*> particles;
    std::vector<DVKCommandBuffer*> threadCommandBuffers;
};

class MyThread
{
public:
    typedef std::function<void()> ThreadFunc;

    explicit MyThread(ThreadFunc func)
        : m_ThreadFunc(func)
        ,m_Thread(func)
    {

    }

    ~MyThread()
    {
        if(m_Thread.joinable())
        {
            m_Thread.join();
        }
    }

    MyThread(MyThread const&) = delete;

    MyThread& operator=(MyThread const&) = delete;

private:
    std::thread m_Thread;
    ThreadFunc  m_ThreadFunc;
};



class ThreadedRenderingDemo :public ModuleBase
{
public:
	ThreadedRenderingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ThreadedRenderingDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
    bool UpdateUI(float time,float delta);
    void UpdateAnimation(float time,float delta);
    void LoadAnimModel();
    void LoadAssets();
    void DestroyAssets();
    void SetupCommandBuffers(int32 backBufferIndex);
    void RenderUI(VkCommandBufferInheritanceInfo inheritanceInfo,int32 backBufferIndex);
    void InitParmas();
    void InitThreads();
    void ThreadRendering(void* param);
    void CreateGUI();
    void DestroyGUI();

private:
    typedef std::vector<DVKCommandBuffer*> CommandBufferArray;

    bool                        m_Ready = false;

    DVKModel* m_Quad = nullptr;

    DVKModel* m_RoleModel = nullptr;
    DVKModel* m_ParticleModel = nullptr;
    DVKShader* m_ParticleShader = nullptr;
    DVKTexture* m_ParticleTexture = nullptr;
    DVKMaterial* m_ParticleMaterial = nullptr;

    CommandBufferArray          m_UICommandBuffers;

    DVKCamera          m_ViewCamera;

    std::mutex                  m_FrameStartLock;
    std::condition_variable     m_FrameStartCV;

    std::mutex                  m_ThreadDoneLock;
    std::condition_variable     m_ThreadDoneCV;
    int32                       m_ThreadDoneCount;

    ModelViewProjectionBlock    m_MVPParam;
    std::vector<Matrix4x4>      m_BonesData;

    std::vector<ParticleModel*> m_Particles;
    std::vector<ThreadData*>    m_ThreadDatas;
    std::vector<MyThread*>      m_Threads;
    bool                        m_ThreadRunning;
    int32                       m_MainFrameID;

    float                       m_FrameTime;
    float                       m_FrameDelta;
    int32                       m_bufferIndex;

    ImageGUIContext* m_GUI = nullptr;
};
