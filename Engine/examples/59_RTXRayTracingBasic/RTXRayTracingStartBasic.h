#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#include <thread>
// Geometry instance, with the layout expected by VK_NV_ray_tracing
struct VkGeometryInstance
{
	// Transform matrix, containing only the top 3 rows
	float transform[12];
	// Instance index
	uint32_t instanceId : 24;
	// Visibility mask
	uint32_t mask : 8;
	// Index of the hit group which will be invoked when a ray hits the instance
	uint32_t instanceOffset : 24;
	// Instance flags, such as culling
	uint32_t flags : 8;
	// Opaque handle of the bottom-level acceleration structure
	uint64_t accelerationStructureHandle;
};
// static_assert(sizeof(VkGeometryInstance) == 64, "VkGeometryInstance structure compiles to incorrect size");
struct CameraParamBlock
{
	Vector4 lens;
	Vector4 pos;
	Matrix4x4 invProj;
	Matrix4x4 invView;
};
class RTXRayTracingStartBasic :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};
public:
	RTXRayTracingStartBasic(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~RTXRayTracingStartBasic();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateUniformBuffer();
	void PrepareDescriptorSets();
	void PrepareShaderBindingTab();
	void PrepareRayTracingPipeline();
	void PrepareUniformBuffers();
	void LoadAssets();
	void LoadExtensions();
	void PrepareAS();
	void CreateTopLevelAS(DVKCommandBuffer* cmdBuffer);
	void CreateBottomLevelAS(DVKCommandBuffer* cmdBuffer,const VkGeometryNV* geometries,int32 geometryCount);
	void DestroyAssets();
	void SetupGfxCommand(int32 backBufferIndex);

private:
	VkPhysicalDeviceFeatures2                           m_EnabledFeatures2;

	PFN_vkCreateAccelerationStructureNV                 vkCreateAccelerationStructureNV;
	PFN_vkDestroyAccelerationStructureNV                vkDestroyAccelerationStructureNV;
	PFN_vkBindAccelerationStructureMemoryNV             vkBindAccelerationStructureMemoryNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV  vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkGetAccelerationStructureHandleNV              vkGetAccelerationStructureHandleNV;
	PFN_vkCmdBuildAccelerationStructureNV               vkCmdBuildAccelerationStructureNV;
	PFN_vkCreateRayTracingPipelinesNV                   vkCreateRayTracingPipelinesNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV             vkGetRayTracingShaderGroupHandlesNV;
	PFN_vkCmdTraceRaysNV                                vkCmdTraceRaysNV;

	VkPhysicalDeviceRayTracingPropertiesNV              m_RayTracingPropertiesNV;

	VkDeviceMemory                                      m_BottomLevelMemory = VK_NULL_HANDLE;
	VkAccelerationStructureNV                           m_BottomLevelAS = VK_NULL_HANDLE;
	uint64                                              m_BottomLevelHandle = 0;

	VkDeviceMemory                                      m_TopLevelMemory = VK_NULL_HANDLE;
	VkAccelerationStructureNV                           m_TopLevelAS = VK_NULL_HANDLE;
	uint64                                              m_TopLevelHandle = 0;

	DVKBuffer* m_ShaderBindingTable = nullptr;
	DVKBuffer* m_UniformBuffer = nullptr;
	CameraParamBlock                                    m_CameraParam;
	DVKCamera                                  m_ViewCamera;

	DVKVertexBuffer* m_VertexBuffer = nullptr;
	DVKIndexBuffer* m_IndexBuffer = nullptr;
	uint32                                              m_IndexCount = 0;

	DVKTexture* m_StorageImage = nullptr;

	VkPipeline                                          m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout                                    m_PipelineLayout = VK_NULL_HANDLE;
	VkDescriptorSet                                     m_DescriptorSet = VK_NULL_HANDLE;
	VkDescriptorSetLayout                               m_DescriptorSetLayout = VK_NULL_HANDLE;
	VkDescriptorPool                                    m_DescriptorPool = VK_NULL_HANDLE;

	DVKModel* m_Quad = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;

	bool                                                m_Ready = false;
};
