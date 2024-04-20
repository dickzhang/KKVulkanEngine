#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define SHADOW_TEX_SIZE 2048
#define INSTANCE_COUNT 512
#define GROUND_RADIUS 15000.0f
class IndirectDrawDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};

	struct CascadeParamBlock
	{
		Matrix4x4 view;
		Vector4   cascadeScale[4];
		Vector4   cascadeOffset[4];
		Matrix4x4 cascadeProj[4];
		Vector4   offset[4];
		Vector4   direction;
		Vector4   bias;
		Vector4   debug;
	};

	struct Triangle
	{
		Vector4 v[3];
		bool    culled = false;
	};

	struct Frustum
	{
		Vector3 origin;               // origin of the frustum (and projection).
		Vector4 orientation;          // Unit quaternion representing rotation.

		float   rightSlope;           // Positive X slope (X/Z).
		float   leftSlope;            // Negative X slope.
		float   topSlope;             // Positive Y slope (Y/Z).
		float   bottomSlope;          // Negative Y slope.
		float   zNear;                // Z of the near plane and far plane.
		float   zFar;                 // Z of the near plane and far plane.
	};

public:
	IndirectDrawDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~IndirectDrawDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void ExtentAABBPoints(Vector4* aabbPoints,const Vector4& center,const Vector4& extent);
	void ComputeFrustumFromProjection(Frustum& out,const Matrix4x4& projection);
	Vector4 VectorSelect(const Vector4& v1,const Vector4& v2,const Vector4& control);
	void CreateFrustumPointsFromCascadeInterval(float cascadeIntervalBegin,float cascadeIntervalEnd,const Matrix4x4& projection,Vector4* cornerPointsWorld);
	Vector4 VectorMin(const Vector4& a,const Vector4& b);
	Vector4 VectorMax(const Vector4& a,const Vector4& b);
	void ComputeNearAndFar(float& nearPlane,float& farPlane,const Vector4& lightCameraOrthographicMin,const Vector4& lightCameraOrthographicMax,Vector4* pointsInCameraView);
	void UpdateCascade();
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void CreateRenderTarget();
	void DestroyRenderTarget();
	void LoadAssets();
	void DestroyAssets();
	void RenderPlantsDepth(VkCommandBuffer commandBuffer);
	void RenderGround(VkCommandBuffer commandBuffer);
	void RenderPlants(VkCommandBuffer commandBuffer);
	void BeginMainPass(VkCommandBuffer commandBuffer,int32 backBufferIndex);
	void RenderDebug(VkCommandBuffer commandBuffer);
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();
private:
	typedef std::vector<DVKTexture*>           TextureArray;
	typedef std::vector<DVKMaterial*>          MaterialArray;
	typedef std::vector<std::vector<DVKMesh*>> MatMeshArray;
	typedef std::vector<VkDrawIndexedIndirectCommand>   IndirectCommandArray;

	bool                        m_Ready = false;

	// Debug
	DVKModel* m_Quad = nullptr;
	DVKMaterial* m_DebugMaterial;
	DVKShader* m_DebugShader;

	// depth
	DVKShader* m_DepthShader = nullptr;
	DVKMaterial* m_DepthMaterial = nullptr;

	DVKShader* m_PCFShadowShader = nullptr;
	DVKMaterial* m_PCFShadowMaterial = nullptr;

	// shadow map
	DVKRenderTarget* m_ShadowRTT = nullptr;
	DVKTexture* m_ShadowMap = nullptr;

	// cascade
	float                       m_CascadePartitionsFrustum[4];
	DVKCamera          m_CascadeCamera[4];
	float                       m_CascadePartitions[4];

	// light camera
	DVKCamera          m_LightCamera;

	// plants
	DVKModel* m_PlantsModel = nullptr;
	DVKShader* m_PlantsShader = nullptr;
	DVKMaterial* m_PlantsMaterial = nullptr;

	IndirectCommandArray        m_IndirectCommands;
	DVKBuffer* m_IndirectCmdBuffer = nullptr;
	DVKBuffer* m_IndirectVertexBuffer = nullptr;
	DVKBuffer* m_IndirectInstanceBuffer = nullptr;
	DVKBuffer* m_IndirectIndexBuffer = nullptr;

	// ground
	DVKModel* m_GroundModel = nullptr;
	DVKShader* m_GroundShader = nullptr;
	DVKMaterial* m_GroundMaterial = nullptr;

	// view
	DVKCamera          m_ViewCamera;

	// params
	ModelViewProjectionBlock    m_MVPParam;
	CascadeParamBlock           m_CascadeParam;

	ImageGUIContext* m_GUI = nullptr;
};
