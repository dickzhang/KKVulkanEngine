#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include "tiny_gltf.h"
#include <vector>
struct Node;
struct Material;
struct Mesh;
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

struct CameraParamBlock
{
	Vector4 lens;
	Vector4 pos;
	IntVector4 samplesAndSeed;
	Matrix4x4 invProj;
	Matrix4x4 invView;
};

struct AccelerationStructureInstance
{
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkAccelerationStructureNV accelerationStructure = VK_NULL_HANDLE;
	uint64 handle = 0;
};

struct Material
{
	Vector4 albedo = Vector4(1.0f,1.0f,1.0f,1.0f); // base color
	Vector4 params = Vector4(1.0f,0.0f,1.0f,0.0f); // roughness, metallic, occlusion, padding
	IntVector4 textureIDs = IntVector4(-1,-1,-1,-1); // albedo, roughness, metallic, padding
};

struct Mesh
{
	uint32 vertexCount = 0;
	uint32 vertexStride = 0;
	DVKBuffer* vertexBuffer = nullptr;

	uint32 indexCount = 0;
	DVKBuffer* indexBuffer = nullptr;

	int32 material = -1;

	~Mesh()
	{
		if(vertexBuffer)
		{
			delete vertexBuffer;
			vertexBuffer = nullptr;
		}

		if(indexBuffer)
		{
			delete indexBuffer;
			indexBuffer = nullptr;
		}
	}
};

struct Node
{
	std::string name;

	Matrix4x4 transform;

	int32 mesh = -1;

	Node* parent = nullptr;
	std::vector<Node*> children;

	Matrix4x4 GetWorldTransform()
	{
		Matrix4x4 worldTransform = transform;
		if(parent)
		{
			worldTransform.Append(parent->GetWorldTransform());
		}
		return worldTransform;
	}
};

struct ObjectInstance
{
	IntVector4 params = IntVector4(-1,-1,-1,-1); // material¡¢mesh¡¢padding¡¢padding
};
struct Scene
{
	Node* rootNode = nullptr;

	std::vector<Node*> nodes;
	std::vector<DVKTexture*> textures;
	std::vector<Material> materials;
	std::vector<Mesh*> meshes;
	std::vector<Node*> entities;

	void Destroy()
	{
		for(int32 i = 0; i<nodes.size(); ++i)
		{
			delete nodes[i];
		}
		nodes.clear();

		for(int32 i = 0; i<textures.size(); ++i)
		{
			delete textures[i];
		}
		textures.clear();

		for(int32 i = 0; i<meshes.size(); ++i)
		{
			delete meshes[i];
		}
		meshes.clear();
	}
};

class RTXRayTracingSimpleDemo :public ModuleBase
{
public:
	RTXRayTracingSimpleDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~RTXRayTracingSimpleDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void UpdateUniformBuffer();
	void PrepareDescriptorSets();
	void PrepareShaderBindingTab();
	void PrepareRayTracingPipeline();
	void PrepareUniformBuffers();
	void LoadTextures(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel);
	void LoadMaterials(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel);
	void LoadMeshes(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel);
	void LoadNode(Node* parent,tinygltf::Node& gltfNode,tinygltf::Model& gltfModel);
	void LoadNodes(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel);
	void LoadGLTFModel(DVKCommandBuffer* cmdBuffer);
	void LoadAssets();
	void LoadExtensions();
	void PrepareAS();
	void CreateTopLevelAS(DVKCommandBuffer* cmdBuffer);
	void CreateBottomLevelAS(DVKCommandBuffer* cmdBuffer);
	void DestroyAssets();
	void SetupGfxCommand(int32 backBufferIndex);
	void CreateGUI();
	void DestroyGUI();
private:
	PFN_vkCreateAccelerationStructureNV                 vkCreateAccelerationStructureNV;
	PFN_vkDestroyAccelerationStructureNV                vkDestroyAccelerationStructureNV;
	PFN_vkBindAccelerationStructureMemoryNV             vkBindAccelerationStructureMemoryNV;
	PFN_vkGetAccelerationStructureMemoryRequirementsNV  vkGetAccelerationStructureMemoryRequirementsNV;
	PFN_vkGetAccelerationStructureHandleNV              vkGetAccelerationStructureHandleNV;
	PFN_vkCmdBuildAccelerationStructureNV               vkCmdBuildAccelerationStructureNV;
	PFN_vkCreateRayTracingPipelinesNV                   vkCreateRayTracingPipelinesNV;
	PFN_vkGetRayTracingShaderGroupHandlesNV             vkGetRayTracingShaderGroupHandlesNV;
	PFN_vkCmdTraceRaysNV                                vkCmdTraceRaysNV;

	VkPhysicalDeviceDescriptorIndexingFeatures          m_IndexingFeatures;
	VkPhysicalDeviceFeatures2                           m_EnabledFeatures2;
	VkPhysicalDeviceRayTracingPropertiesNV              m_RayTracingPropertiesNV;

	std::vector<AccelerationStructureInstance>          m_BottomLevelsAS;
	AccelerationStructureInstance                       m_TopLevelAS;

	DVKBuffer* m_ShaderBindingTable = nullptr;
	DVKBuffer* m_UniformBuffer = nullptr;
	CameraParamBlock                                    m_CameraParam;
	DVKCamera                                  m_ViewCamera;

	DVKTexture* m_StorageImage = nullptr;

	VkPipeline                                          m_Pipeline = VK_NULL_HANDLE;
	VkPipelineLayout                                    m_PipelineLayout = VK_NULL_HANDLE;
	std::vector<VkDescriptorSet>                        m_DescriptorSets;
	std::vector<VkDescriptorSetLayout>                  m_DescriptorSetLayouts;
	VkDescriptorPool                                    m_DescriptorPool = VK_NULL_HANDLE;

	DVKModel* m_Quad = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;

	Scene                                               m_Scene;
	DVKBuffer* m_MaterialsBuffer = nullptr;
	DVKBuffer* m_ObjectsBuffer = nullptr;

	bool                                                m_Ready = false;

	ImageGUIContext* m_GUI = nullptr;
};
