#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class ComputeShaderDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};

	struct ComputeResource
	{
		VkCommandPool                   commandPool;

		VkDescriptorPool                descriptorPool;
		VkDescriptorSetLayout           descriptorSetLayout;
		VkPipelineLayout                pipelineLayout;

		VkDescriptorSet                 descriptorSets[3];
		VkPipeline                      pipelines[3];
		DVKTexture* targets[3];

		void Destroy(VkDevice device)
		{
			vkDestroyCommandPool(device,commandPool,VULKAN_CPU_ALLOCATOR);

			vkDestroyDescriptorPool(device,descriptorPool,VULKAN_CPU_ALLOCATOR);
			vkDestroyDescriptorSetLayout(device,descriptorSetLayout,VULKAN_CPU_ALLOCATOR);
			vkDestroyPipelineLayout(device,pipelineLayout,VULKAN_CPU_ALLOCATOR);

			for(int32 i = 0; i<3; ++i)
			{
				vkDestroyPipeline(device,pipelines[i],VULKAN_CPU_ALLOCATOR);
				delete targets[i];
			}
		}
	};
public:
	ComputeShaderDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ComputeShaderDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void ProcessImage();
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                        m_Ready = false;
	DVKModel* m_ModelPlane = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKTexture* m_Texture = nullptr;

	DVKCamera          m_ViewCamera;
	ModelViewProjectionBlock    m_MVPParam;
	ComputeResource             m_ComputeRes;
	std::vector<const char*>    m_FilterNames;
	int32                       m_FilterIndex;
	ImageGUIContext* m_GUI = nullptr;
};
