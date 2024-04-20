#include "RTXPathTracingDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<RTXPathTracingDemo>(1400,900,"RTXPathTracingDemo",cmdLine);
}

RTXPathTracingDemo::RTXPathTracingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
	deviceExtensions.push_back(VK_NV_RAY_TRACING_EXTENSION_NAME);
	deviceExtensions.push_back(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);
	deviceExtensions.push_back(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME);
	instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	ZeroVulkanStruct(m_IndexingFeatures,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES);
	m_IndexingFeatures.pNext = nullptr;
	m_IndexingFeatures.runtimeDescriptorArray = true;

	ZeroVulkanStruct(m_EnabledFeatures2,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2);
	m_EnabledFeatures2.pNext = &m_IndexingFeatures;

	physicalDeviceFeatures = &m_EnabledFeatures2;
}

RTXPathTracingDemo::~RTXPathTracingDemo()
{

}
bool RTXPathTracingDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	LoadExtensions();
	LoadAssets();
	PrepareUniformBuffers();
	PrepareRayTracingPipeline();
	PrepareShaderBindingTab();
	PrepareAS();
	PrepareDescriptorSets();

	m_Ready = true;

	return true;
}

void RTXPathTracingDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void RTXPathTracingDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void RTXPathTracingDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	if(InputManager::IsMouseDown(MouseType::MOUSE_BUTTON_LEFT)||
		InputManager::GetMouseDelta()!=0||
		InputManager::IsKeyDown(KeyboardType::KEY_SPACE)
		)
	{
		m_GlobalParam.moving.y = 0;
		m_GlobalParam.moving.x = 1;
	}
	else
	{
		m_GlobalParam.moving.x = 0;
	}

	m_GlobalParam.moving.y = m_GlobalParam.moving.y+1;

	UpdateUniformBuffer();
	SetupGfxCommand(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool RTXPathTracingDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("RTXRaytracing",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		// MaxBounces
		{
			int maxBounces = m_GlobalParam.samplingData.z;
			ImGui::SliderInt("MaxBounces",&maxBounces,1,128);
			m_GlobalParam.samplingData.z = maxBounces;
		}

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void RTXPathTracingDemo::UpdateUniformBuffer()
{
	m_GlobalParam.samplingData.x = m_GlobalParam.samplingData.x+1;
	m_GlobalParam.samplingData.y = MMath::Atan((2.0f*MMath::Tan(m_ViewCamera.GetFov()*0.5f))/m_FrameHeight);

	m_GlobalParam.pos = m_ViewCamera.GetTransform().GetOrigin();

	m_GlobalParam.invProj = m_ViewCamera.GetProjection();
	m_GlobalParam.invProj.SetInverse();
	m_GlobalParam.invView = m_ViewCamera.GetView();
	m_GlobalParam.invView.SetInverse();

	memcpy(m_UniformBuffer->mapped,&m_GlobalParam,sizeof(GlobalParamBlock));
}

void RTXPathTracingDemo::PrepareDescriptorSets()
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	m_DescriptorSets.resize(m_DescriptorSetLayouts.size());

	// sets
	std::vector<VkDescriptorPoolSize> poolSizes(7);
	// set=0,accelerationStructureNV
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	poolSizes[0].descriptorCount = 1;
	// set=0,image
	poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	poolSizes[1].descriptorCount = 1;
	// set=0,global params
	poolSizes[2].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[2].descriptorCount = 1;
	// set=1,Vertices + Indices
	poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[3].descriptorCount = 2*m_Scene.meshes.size();
	// set=1,Materials + ObjectInstances
	poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[4].descriptorCount = 2;
	// set=1,textures
	poolSizes[5].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	poolSizes[5].descriptorCount = 1*m_Scene.meshes.size();
	// set=0,lights
	poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	poolSizes[6].descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo;
	ZeroVulkanStruct(descriptorPoolCreateInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
	descriptorPoolCreateInfo.poolSizeCount = poolSizes.size();
	descriptorPoolCreateInfo.pPoolSizes = poolSizes.data();
	descriptorPoolCreateInfo.maxSets = m_DescriptorSets.size();
	VERIFYVULKANRESULT(vkCreateDescriptorPool(device,&descriptorPoolCreateInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorPool));

	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo;
	ZeroVulkanStruct(descriptorSetAllocateInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
	descriptorSetAllocateInfo.descriptorPool = m_DescriptorPool;
	descriptorSetAllocateInfo.pSetLayouts = m_DescriptorSetLayouts.data();
	descriptorSetAllocateInfo.descriptorSetCount = m_DescriptorSetLayouts.size();
	VERIFYVULKANRESULT(vkAllocateDescriptorSets(device,&descriptorSetAllocateInfo,m_DescriptorSets.data()));

	std::vector<VkWriteDescriptorSet> writeDescriptorSets;

	// set0
	// topLevelAS
	VkWriteDescriptorSetAccelerationStructureNV descriptorASNV;
	ZeroVulkanStruct(descriptorASNV,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_NV);
	descriptorASNV.accelerationStructureCount = 1;
	descriptorASNV.pAccelerationStructures = &m_TopLevelAS.accelerationStructure;
	VkWriteDescriptorSet asWriteDescriptorSet;
	ZeroVulkanStruct(asWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	asWriteDescriptorSet.pNext = &descriptorASNV;
	asWriteDescriptorSet.dstSet = m_DescriptorSets[0];
	asWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
	asWriteDescriptorSet.dstBinding = 0;
	asWriteDescriptorSet.descriptorCount = 1;

	// image
	VkDescriptorImageInfo imageInfo;
	imageInfo.imageView = m_StorageImage->imageView;
	imageInfo.imageLayout = m_StorageImage->imageLayout;
	VkWriteDescriptorSet imageWriteDescriptorSet;
	ZeroVulkanStruct(imageWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	imageWriteDescriptorSet.pImageInfo = &imageInfo;
	imageWriteDescriptorSet.dstSet = m_DescriptorSets[0];
	imageWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
	imageWriteDescriptorSet.dstBinding = 1;
	imageWriteDescriptorSet.descriptorCount = 1;

	// global params
	VkWriteDescriptorSet uboWriteDescriptorSet;
	ZeroVulkanStruct(uboWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	uboWriteDescriptorSet.pBufferInfo = &m_UniformBuffer->descriptor;
	uboWriteDescriptorSet.dstSet = m_DescriptorSets[0];
	uboWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboWriteDescriptorSet.dstBinding = 2;
	uboWriteDescriptorSet.descriptorCount = 1;

	// lights
	VkWriteDescriptorSet lightWriteDescriptorSet;
	ZeroVulkanStruct(lightWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	lightWriteDescriptorSet.pBufferInfo = &m_LightsBuffer->descriptor;
	lightWriteDescriptorSet.dstSet = m_DescriptorSets[0];
	lightWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	lightWriteDescriptorSet.dstBinding = 3;
	lightWriteDescriptorSet.descriptorCount = 1;

	writeDescriptorSets.push_back(asWriteDescriptorSet);
	writeDescriptorSets.push_back(imageWriteDescriptorSet);
	writeDescriptorSets.push_back(uboWriteDescriptorSet);
	writeDescriptorSets.push_back(lightWriteDescriptorSet);

	// set 1
	// vertices
	std::vector<VkDescriptorBufferInfo> vertexBufferInfos(m_Scene.meshes.size());
	for(int32 i = 0; i<m_Scene.meshes.size(); ++i)
	{
		vertexBufferInfos[i].buffer = m_Scene.meshes[i]->vertexBuffer->buffer;
		vertexBufferInfos[i].offset = 0;
		vertexBufferInfos[i].range = VK_WHOLE_SIZE;
	}
	VkWriteDescriptorSet vertexWriteDescriptorSet;
	ZeroVulkanStruct(vertexWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	vertexWriteDescriptorSet.pBufferInfo = vertexBufferInfos.data();
	vertexWriteDescriptorSet.dstSet = m_DescriptorSets[1];
	vertexWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	vertexWriteDescriptorSet.dstBinding = 0;
	vertexWriteDescriptorSet.descriptorCount = vertexBufferInfos.size();

	// indices
	std::vector<VkDescriptorBufferInfo> indexBufferInfos(m_Scene.meshes.size());
	for(int32 i = 0; i<m_Scene.meshes.size(); ++i)
	{
		indexBufferInfos[i].buffer = m_Scene.meshes[i]->indexBuffer->buffer;
		indexBufferInfos[i].offset = 0;
		indexBufferInfos[i].range = VK_WHOLE_SIZE;
	}
	VkWriteDescriptorSet indexWriteDescriptorSet;
	ZeroVulkanStruct(indexWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	indexWriteDescriptorSet.pBufferInfo = indexBufferInfos.data();
	indexWriteDescriptorSet.dstSet = m_DescriptorSets[1];
	indexWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	indexWriteDescriptorSet.dstBinding = 1;
	indexWriteDescriptorSet.descriptorCount = indexBufferInfos.size();

	// materials
	VkDescriptorBufferInfo materialsBufferInfo;
	materialsBufferInfo.buffer = m_MaterialsBuffer->buffer;
	materialsBufferInfo.offset = 0;
	materialsBufferInfo.range = VK_WHOLE_SIZE;
	VkWriteDescriptorSet materiaslWriteDescriptorSet;
	ZeroVulkanStruct(materiaslWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	materiaslWriteDescriptorSet.pBufferInfo = &materialsBufferInfo;
	materiaslWriteDescriptorSet.dstSet = m_DescriptorSets[1];
	materiaslWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	materiaslWriteDescriptorSet.dstBinding = 2;
	materiaslWriteDescriptorSet.descriptorCount = 1;

	// objects
	VkDescriptorBufferInfo objectsBufferInfo;
	objectsBufferInfo.buffer = m_ObjectsBuffer->buffer;
	objectsBufferInfo.offset = 0;
	objectsBufferInfo.range = VK_WHOLE_SIZE;
	VkWriteDescriptorSet objectsWriteDescriptorSet;
	ZeroVulkanStruct(objectsWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	objectsWriteDescriptorSet.pBufferInfo = &objectsBufferInfo;
	objectsWriteDescriptorSet.dstSet = m_DescriptorSets[1];
	objectsWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	objectsWriteDescriptorSet.dstBinding = 3;
	objectsWriteDescriptorSet.descriptorCount = 1;

	// textures
	std::vector<VkDescriptorImageInfo> textureImageInfos(m_Scene.textures.size());
	for(int32 i = 0; i<m_Scene.textures.size(); ++i)
	{
		textureImageInfos[i] = m_Scene.textures[i]->descriptorInfo;
	}
	VkWriteDescriptorSet textureWriteDescriptorSet;
	ZeroVulkanStruct(textureWriteDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	textureWriteDescriptorSet.pImageInfo = textureImageInfos.data();
	textureWriteDescriptorSet.dstSet = m_DescriptorSets[1];
	textureWriteDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	textureWriteDescriptorSet.dstBinding = 4;
	textureWriteDescriptorSet.descriptorCount = m_Scene.textures.size();

	writeDescriptorSets.push_back(vertexWriteDescriptorSet);
	writeDescriptorSets.push_back(indexWriteDescriptorSet);
	writeDescriptorSets.push_back(materiaslWriteDescriptorSet);
	writeDescriptorSets.push_back(objectsWriteDescriptorSet);
	writeDescriptorSets.push_back(textureWriteDescriptorSet);

	vkUpdateDescriptorSets(device,writeDescriptorSets.size(),writeDescriptorSets.data(),0,nullptr);
}

void RTXPathTracingDemo::PrepareShaderBindingTab()
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	const uint32 shaderGroupHandleSize = Align(m_RayTracingPropertiesNV.shaderGroupHandleSize,m_RayTracingPropertiesNV.shaderGroupBaseAlignment);
	const uint32 shaderGroupTotalSize = shaderGroupHandleSize*3;
	std::vector<uint8> shaderGroupHandleData(shaderGroupTotalSize);

	VERIFYVULKANRESULT(vkGetRayTracingShaderGroupHandlesNV(device,m_Pipeline,0,1,shaderGroupHandleSize,shaderGroupHandleData.data()+shaderGroupHandleSize*0));
	VERIFYVULKANRESULT(vkGetRayTracingShaderGroupHandlesNV(device,m_Pipeline,1,1,shaderGroupHandleSize,shaderGroupHandleData.data()+shaderGroupHandleSize*1));
	VERIFYVULKANRESULT(vkGetRayTracingShaderGroupHandlesNV(device,m_Pipeline,2,1,shaderGroupHandleSize,shaderGroupHandleData.data()+shaderGroupHandleSize*2));

	m_ShaderBindingTable = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		shaderGroupTotalSize,
		shaderGroupHandleData.data()
	);
}

void RTXPathTracingDemo::PrepareRayTracingPipeline()
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	m_DescriptorSetLayouts.resize(2);

	// set0
	{
		// topLevelAS
		VkDescriptorSetLayoutBinding asLayoutBinding = { };
		asLayoutBinding.binding = 0;
		asLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV;
		asLayoutBinding.descriptorCount = 1;
		asLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		// image
		VkDescriptorSetLayoutBinding imageLayoutBinding = { };
		imageLayoutBinding.binding = 1;
		imageLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		imageLayoutBinding.descriptorCount = 1;
		imageLayoutBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		// global params
		VkDescriptorSetLayoutBinding uniformBufferBinding = { };
		uniformBufferBinding.binding = 2;
		uniformBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uniformBufferBinding.descriptorCount = 1;
		uniformBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;
		// lights
		VkDescriptorSetLayoutBinding lightBufferBinding = { };
		lightBufferBinding.binding = 3;
		lightBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		lightBufferBinding.descriptorCount = 1;
		lightBufferBinding.stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_NV;

		std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
		layoutBindings.push_back(asLayoutBinding);
		layoutBindings.push_back(imageLayoutBinding);
		layoutBindings.push_back(uniformBufferBinding);
		layoutBindings.push_back(lightBufferBinding);

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
		ZeroVulkanStruct(layoutCreateInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
		layoutCreateInfo.bindingCount = layoutBindings.size();
		layoutCreateInfo.pBindings = layoutBindings.data();
		VERIFYVULKANRESULT(vkCreateDescriptorSetLayout(device,&layoutCreateInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorSetLayouts[0]));
	}

	// set1
	{
		std::vector<VkDescriptorSetLayoutBinding> layoutBindings(5);
		// Vertices
		layoutBindings[0].binding = 0;
		layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindings[0].descriptorCount = m_Scene.meshes.size();
		layoutBindings[0].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		// Indices
		layoutBindings[1].binding = 1;
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindings[1].descriptorCount = m_Scene.meshes.size();
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		// Materials
		layoutBindings[2].binding = 2;
		layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindings[2].descriptorCount = 1;
		layoutBindings[2].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		// ObjectInstances
		layoutBindings[3].binding = 3;
		layoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBindings[3].descriptorCount = 1;
		layoutBindings[3].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
		// textures
		layoutBindings[4].binding = 4;
		layoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[4].descriptorCount = m_Scene.textures.size();
		layoutBindings[4].stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;

		VkDescriptorSetLayoutCreateInfo layoutCreateInfo;
		ZeroVulkanStruct(layoutCreateInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
		layoutCreateInfo.bindingCount = layoutBindings.size();
		layoutCreateInfo.pBindings = layoutBindings.data();
		VERIFYVULKANRESULT(vkCreateDescriptorSetLayout(device,&layoutCreateInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorSetLayouts[1]));
	}

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
	ZeroVulkanStruct(pipelineLayoutCreateInfo,VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
	pipelineLayoutCreateInfo.setLayoutCount = m_DescriptorSetLayouts.size();
	pipelineLayoutCreateInfo.pSetLayouts = m_DescriptorSetLayouts.data();
	VERIFYVULKANRESULT(vkCreatePipelineLayout(device,&pipelineLayoutCreateInfo,VULKAN_CPU_ALLOCATOR,&m_PipelineLayout));

	auto rayGenShaderModule = DVKShaderModule::Create(m_VulkanDevice,"Assets/Shaders/68_RTXPathTracing/raygen.rgen.spv",VK_SHADER_STAGE_RAYGEN_BIT_NV);
	auto rayMisShaderModule = DVKShaderModule::Create(m_VulkanDevice,"Assets/Shaders/68_RTXPathTracing/miss.rmiss.spv",VK_SHADER_STAGE_MISS_BIT_NV);
	auto rayHitShaderModule = DVKShaderModule::Create(m_VulkanDevice,"Assets/Shaders/68_RTXPathTracing/closesthit.rchit.spv",VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV);

	std::vector<VkPipelineShaderStageCreateInfo> Shaderstages(3);
	Shaderstages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	Shaderstages[0].pNext = nullptr;
	Shaderstages[0].flags = 0;
	Shaderstages[0].stage = VK_SHADER_STAGE_RAYGEN_BIT_NV;
	Shaderstages[0].module = rayGenShaderModule->handle;
	Shaderstages[0].pName = "main";
	Shaderstages[0].pSpecializationInfo = nullptr;

	Shaderstages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	Shaderstages[1].pNext = nullptr;
	Shaderstages[1].flags = 0;
	Shaderstages[1].stage = VK_SHADER_STAGE_MISS_BIT_NV;
	Shaderstages[1].module = rayMisShaderModule->handle;
	Shaderstages[1].pName = "main";
	Shaderstages[1].pSpecializationInfo = nullptr;

	Shaderstages[2].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	Shaderstages[2].pNext = nullptr;
	Shaderstages[2].flags = 0;
	Shaderstages[2].stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_NV;
	Shaderstages[2].module = rayHitShaderModule->handle;
	Shaderstages[2].pName = "main";
	Shaderstages[2].pSpecializationInfo = nullptr;

	std::vector<VkRayTracingShaderGroupCreateInfoNV> shaderGroups(3);
	shaderGroups[0].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroups[0].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	shaderGroups[0].pNext = nullptr;
	shaderGroups[0].generalShader = 0;
	shaderGroups[0].closestHitShader = VK_SHADER_UNUSED_NV;
	shaderGroups[0].anyHitShader = VK_SHADER_UNUSED_NV;
	shaderGroups[0].intersectionShader = VK_SHADER_UNUSED_NV;

	shaderGroups[1].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroups[1].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_NV;
	shaderGroups[1].pNext = nullptr;
	shaderGroups[1].generalShader = 1;
	shaderGroups[1].closestHitShader = VK_SHADER_UNUSED_NV;
	shaderGroups[1].anyHitShader = VK_SHADER_UNUSED_NV;
	shaderGroups[1].intersectionShader = VK_SHADER_UNUSED_NV;

	shaderGroups[2].sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_NV;
	shaderGroups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_NV;
	shaderGroups[2].pNext = nullptr;
	shaderGroups[2].generalShader = VK_SHADER_UNUSED_NV;
	shaderGroups[2].closestHitShader = 2;
	shaderGroups[2].anyHitShader = VK_SHADER_UNUSED_NV;
	shaderGroups[2].intersectionShader = VK_SHADER_UNUSED_NV;

	VkRayTracingPipelineCreateInfoNV pipelineCreateInfo;
	ZeroVulkanStruct(pipelineCreateInfo,VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_NV);
	pipelineCreateInfo.stageCount = Shaderstages.size();
	pipelineCreateInfo.pStages = Shaderstages.data();
	pipelineCreateInfo.groupCount = shaderGroups.size();
	pipelineCreateInfo.pGroups = shaderGroups.data();
	pipelineCreateInfo.maxRecursionDepth = 8;
	pipelineCreateInfo.layout = m_PipelineLayout;
	VERIFYVULKANRESULT(vkCreateRayTracingPipelinesNV(device,VK_NULL_HANDLE,1,&pipelineCreateInfo,VULKAN_CPU_ALLOCATOR,&m_Pipeline));

	delete rayGenShaderModule;
	delete rayMisShaderModule;
	delete rayHitShaderModule;
}

void RTXPathTracingDemo::PrepareUniformBuffers()
{
	m_GlobalParam.viewSize.x = m_FrameWidth;
	m_GlobalParam.viewSize.y = m_FrameHeight;
	m_GlobalParam.viewSize.z = 1.0f/m_FrameWidth;
	m_GlobalParam.viewSize.w = 1.0f/m_FrameHeight;

	m_GlobalParam.samplingData.x = 0;
	m_GlobalParam.samplingData.y = MMath::Atan((2.0f*MMath::Tan(m_ViewCamera.GetFov()*0.5f))/m_FrameHeight);
	m_GlobalParam.samplingData.z = 8;
	m_GlobalParam.samplingData.w = 0.1f;

	m_GlobalParam.lightInfo.x = m_Scene.lights.size();

	m_UniformBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(GlobalParamBlock),
		&m_GlobalParam
	);
	m_UniformBuffer->Map();

	m_ViewCamera.SetPosition(Vector3(2.97830200f,42.5552597f,53.1424141f));
	m_ViewCamera.SetRotation(Vector3(-145.999985f,-1.49999976f,180.000000f));
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),0.1f,1000.0f);
}

void RTXPathTracingDemo::LoadLights(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel)
{
	// default sky light
	m_Scene.lights.push_back(new SkyEnvLight(Vector3(0.5f,0.5f,1.0f)));

	for(int32 i = 0; i<gltfModel.lights.size(); ++i)
	{
		tinygltf::Light& light = gltfModel.lights[i];
	}

	if(gltfModel.lights.size()==0)
	{
		DirectionalLight* defaultLight = new DirectionalLight(Vector3(1,-1,1).GetSafeNormal(),Vector3(5,5,5));
		m_Scene.lights.push_back(defaultLight);
	}
}

void RTXPathTracingDemo::LoadTextures(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel)
{
	for(int32 i = 0; i<gltfModel.textures.size(); ++i)
	{
		tinygltf::Texture& tex = gltfModel.textures[i];
		tinygltf::Image& image = gltfModel.images[tex.source];
		DVKTexture* texture = DVKTexture::Create2D(image.image.data(),image.width*image.height*4,VK_FORMAT_R8G8B8A8_UNORM,image.width,image.height,m_VulkanDevice,cmdBuffer);
		m_Scene.textures.push_back(texture);
	}
}

void RTXPathTracingDemo::LoadMaterials(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel)
{
	for(int32 i = 0; i<gltfModel.materials.size(); ++i)
	{
		tinygltf::Material& gltfMat = gltfModel.materials[i];
		Material material;

		if(gltfMat.values.find("baseColorTexture")!=gltfMat.values.end())
		{
			material.textureIDs.x = gltfMat.values["baseColorTexture"].TextureIndex();
		}

		if(gltfMat.values.find("roughnessFactor")!=gltfMat.values.end())
		{
			material.params.x = gltfMat.values["roughnessFactor"].Factor();
		}

		if(gltfMat.values.find("metallicFactor")!=gltfMat.values.end())
		{
			material.params.y = gltfMat.values["metallicFactor"].Factor();
		}

		if(gltfMat.values.find("baseColorFactor")!=gltfMat.values.end())
		{
			material.albedo.x = gltfMat.values["baseColorFactor"].ColorFactor().data()[0];
			material.albedo.y = gltfMat.values["baseColorFactor"].ColorFactor().data()[1];
			material.albedo.z = gltfMat.values["baseColorFactor"].ColorFactor().data()[2];
			material.albedo.w = gltfMat.values["baseColorFactor"].ColorFactor().data()[3];
		}

		m_Scene.materials.push_back(material);
	}
}

void RTXPathTracingDemo::LoadMeshes(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel)
{
	for(int32 i = 0; i<gltfModel.meshes.size(); ++i)
	{
		tinygltf::Mesh& gltfMesh = gltfModel.meshes[i];

		for(int32 j = 0; j<gltfMesh.primitives.size(); ++j)
		{
			tinygltf::Primitive& primitive = gltfMesh.primitives[j];

			std::vector<float> vertices;
			std::vector<uint32> indices;
			BoundingBox bounds;

			// vertices
			uint8* bufferPos = nullptr;
			uint8* bufferNormals = nullptr;
			uint8* bufferUV0 = nullptr;
			uint8* bufferTangents = nullptr;

			tinygltf::Accessor& posAccessor = gltfModel.accessors[primitive.attributes.find("POSITION")->second];
			tinygltf::BufferView& posView = gltfModel.bufferViews[posAccessor.bufferView];
			bufferPos = &(gltfModel.buffers[posView.buffer].data[posAccessor.byteOffset+posView.byteOffset]);

			if(primitive.attributes.find("NORMAL")!=primitive.attributes.end())
			{
				tinygltf::Accessor& normAccessor = gltfModel.accessors[primitive.attributes.find("NORMAL")->second];
				tinygltf::BufferView& normView = gltfModel.bufferViews[normAccessor.bufferView];
				bufferNormals = &(gltfModel.buffers[normView.buffer].data[normAccessor.byteOffset+normView.byteOffset]);
			}

			if(primitive.attributes.find("TEXCOORD_0")!=primitive.attributes.end())
			{
				tinygltf::Accessor& uvAccessor = gltfModel.accessors[primitive.attributes.find("TEXCOORD_0")->second];
				tinygltf::BufferView& uvView = gltfModel.bufferViews[uvAccessor.bufferView];
				bufferUV0 = &(gltfModel.buffers[uvView.buffer].data[uvAccessor.byteOffset+uvView.byteOffset]);
			}

			if(primitive.attributes.find("TANGENT")!=primitive.attributes.end())
			{
				tinygltf::Accessor& tangentAccessor = gltfModel.accessors[primitive.attributes.find("TANGENT")->second];
				tinygltf::BufferView& tangentView = gltfModel.bufferViews[tangentAccessor.bufferView];
				bufferTangents = &(gltfModel.buffers[tangentView.buffer].data[tangentAccessor.byteOffset+tangentView.byteOffset]);
			}

			for(int32 v = 0; v<posAccessor.count; ++v)
			{
				// pos
				{
					const float* buf = (const float*)(bufferPos);
					float px = buf[v*3+0];
					float py = buf[v*3+1];
					float pz = buf[v*3+2];

					if(v==0)
					{
						bounds.min.x = bounds.max.x = px;
						bounds.min.y = bounds.max.y = py;
						bounds.min.z = bounds.max.z = pz;
					}

					bounds.min.x = MMath::Min(bounds.min.x,px);
					bounds.min.y = MMath::Min(bounds.min.y,py);
					bounds.min.z = MMath::Min(bounds.min.z,pz);
					bounds.max.x = MMath::Max(bounds.max.x,px);
					bounds.max.y = MMath::Max(bounds.max.y,py);
					bounds.max.z = MMath::Max(bounds.max.z,pz);

					vertices.push_back(px);
					vertices.push_back(py);
					vertices.push_back(pz);
				}
				// uv
				{
					if(bufferUV0)
					{
						const float* buf = (const float*)(bufferUV0);
						vertices.push_back(buf[v*2+0]);
						vertices.push_back(buf[v*2+1]);
					}
					else
					{
						vertices.push_back(0);
						vertices.push_back(0);
					}
				}
				// normal
				{
					if(bufferNormals)
					{
						const float* buf = (const float*)(bufferNormals);
						vertices.push_back(buf[v*3+0]);
						vertices.push_back(buf[v*3+1]);
						vertices.push_back(buf[v*3+2]);
					}
					else
					{
						vertices.push_back(0);
						vertices.push_back(0);
						vertices.push_back(0);
					}
				}
				// tangent
				{
					if(bufferTangents)
					{
						const float* buf = (const float*)(bufferTangents);
						vertices.push_back(buf[v*3+0]);
						vertices.push_back(buf[v*3+1]);
						vertices.push_back(buf[v*3+2]);
					}
					else
					{
						vertices.push_back(0);
						vertices.push_back(0);
						vertices.push_back(0);
					}
				}
			}

			// indices
			tinygltf::Accessor& indicesAccessor = gltfModel.accessors[primitive.indices];
			tinygltf::BufferView& indicesBufferView = gltfModel.bufferViews[indicesAccessor.bufferView];
			uint8* bufferIndices = &(gltfModel.buffers[indicesBufferView.buffer].data[indicesAccessor.byteOffset+indicesBufferView.byteOffset]);

			for(int32 v = 0; v<indicesAccessor.count; ++v)
			{
				if(indicesAccessor.componentType==TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
				{
					const uint32* buf = (const uint32*)(bufferIndices);
					indices.push_back(buf[v]);
				}
				else if(indicesAccessor.componentType==TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
				{
					const uint16* buf = (const uint16*)(bufferIndices);
					indices.push_back(buf[v]);
				}
				else if(indicesAccessor.componentType==TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
				{
					const uint8* buf = (const uint8*)(bufferIndices);
					indices.push_back(buf[v]);
				}
			}

			Mesh* mesh = new Mesh();
			mesh->bounds = bounds;
			mesh->vertexStride = 11*sizeof(float);
			mesh->vertexCount = vertices.size();
			{
				DVKBuffer* vertexStaging = DVKBuffer::CreateBuffer(
					m_VulkanDevice,
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					vertices.size()*sizeof(float),
					vertices.data()
				);

				mesh->vertexBuffer = DVKBuffer::CreateBuffer(
					m_VulkanDevice,
					VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					vertices.size()*sizeof(float)
				);

				cmdBuffer->Begin();

				VkBufferCopy copyRegion = { };
				copyRegion.size = vertices.size()*sizeof(float);
				vkCmdCopyBuffer(cmdBuffer->cmdBuffer,vertexStaging->buffer,mesh->vertexBuffer->buffer,1,&copyRegion);

				cmdBuffer->End();
				cmdBuffer->Submit();

				delete vertexStaging;
			}

			mesh->indexCount = indices.size();
			{
				DVKBuffer* indexStaging = DVKBuffer::CreateBuffer(
					m_VulkanDevice,
					VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					indices.size()*sizeof(uint32),
					indices.data()
				);

				mesh->indexBuffer = DVKBuffer::CreateBuffer(
					m_VulkanDevice,
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT|VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
					VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
					indices.size()*sizeof(uint32)
				);

				cmdBuffer->Begin();

				VkBufferCopy copyRegion = { };
				copyRegion.size = indices.size()*sizeof(uint32);

				vkCmdCopyBuffer(cmdBuffer->cmdBuffer,indexStaging->buffer,mesh->indexBuffer->buffer,1,&copyRegion);

				cmdBuffer->End();
				cmdBuffer->Submit();

				delete indexStaging;
			}

			mesh->material = primitive.material;
			m_Scene.meshes.push_back(mesh);
		}
	}
}

void RTXPathTracingDemo::LoadNode(Node* parent,tinygltf::Node& gltfNode,tinygltf::Model& gltfModel)
{
	Node* node = new Node();
	node->name = gltfNode.name;
	node->transform.SetIdentity();

	m_Scene.nodes.push_back(node);

	if(gltfNode.rotation.size()==4)
	{
		Quat quat(gltfNode.rotation[0],gltfNode.rotation[1],gltfNode.rotation[2],gltfNode.rotation[3]);
		node->transform.Append(quat.ToMatrix());
	}

	if(gltfNode.scale.size()==3)
	{
		node->transform.AppendScale(Vector3(gltfNode.scale[0],gltfNode.scale[1],gltfNode.scale[2]));
	}

	if(gltfNode.translation.size()==3)
	{
		node->transform.AppendTranslation(Vector3(gltfNode.translation[0],gltfNode.translation[1],gltfNode.translation[2]));
	}

	if(gltfNode.matrix.size()==16)
	{
		node->transform.m[0][0] = (float)gltfNode.matrix[0];
		node->transform.m[0][1] = (float)gltfNode.matrix[1];
		node->transform.m[0][2] = (float)gltfNode.matrix[2];
		node->transform.m[0][3] = (float)gltfNode.matrix[3];

		node->transform.m[1][0] = (float)gltfNode.matrix[4];
		node->transform.m[1][1] = (float)gltfNode.matrix[5];
		node->transform.m[1][2] = (float)gltfNode.matrix[6];
		node->transform.m[1][3] = (float)gltfNode.matrix[7];

		node->transform.m[2][0] = (float)gltfNode.matrix[8];
		node->transform.m[2][1] = (float)gltfNode.matrix[9];
		node->transform.m[2][2] = (float)gltfNode.matrix[10];
		node->transform.m[2][3] = (float)gltfNode.matrix[11];

		node->transform.m[3][0] = (float)gltfNode.matrix[12];
		node->transform.m[3][1] = (float)gltfNode.matrix[13];
		node->transform.m[3][2] = (float)gltfNode.matrix[14];
		node->transform.m[3][3] = (float)gltfNode.matrix[15];
	}

	node->parent = parent;
	if(parent)
	{
		parent->children.push_back(node);
	}

	if(gltfNode.mesh>-1)
	{
		node->mesh = gltfNode.mesh;

		m_Scene.entities.push_back(node);
	}

	for(int32 i = 0; i<gltfNode.children.size(); ++i)
	{
		LoadNode(node,gltfModel.nodes[gltfNode.children[i]],gltfModel);
	}
}

void RTXPathTracingDemo::LoadNodes(DVKCommandBuffer* cmdBuffer,tinygltf::Model& gltfModel)
{
	tinygltf::Scene& scene = gltfModel.scenes[0];

	m_Scene.rootNode = new Node();

	for(int32 i = 0; i<scene.nodes.size(); ++i)
	{
		tinygltf::Node& gltfNode = gltfModel.nodes[scene.nodes[i]];
		LoadNode(m_Scene.rootNode,gltfNode,gltfModel);
	}
}

void RTXPathTracingDemo::LoadGLTFModel(DVKCommandBuffer* cmdBuffer)
{
	tinygltf::Model gltfModel;
	tinygltf::TinyGLTF gltfContext;
	std::string error;
	std::string warning;

	uint32 dataSize = 0;
	uint8* dataPtr = nullptr;
	FileManager::ReadFile("Assets/models/diorama/diorama.glb",dataPtr,dataSize);

	gltfContext.LoadBinaryFromMemory(&gltfModel,&error,&warning,dataPtr,dataSize);

	LoadTextures(cmdBuffer,gltfModel);
	LoadMaterials(cmdBuffer,gltfModel);
	LoadMeshes(cmdBuffer,gltfModel);
	LoadNodes(cmdBuffer,gltfModel);
	LoadLights(cmdBuffer,gltfModel);
}

void RTXPathTracingDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	LoadGLTFModel(cmdBuffer);

	// prepare light buffer
	m_Scene.UpdateLightData();
	m_LightsBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(LightData)*m_Scene.lightDatas.size(),
		m_Scene.lightDatas.data()
	);

	// prepare material buffer
	m_MaterialsBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(Material)*m_Scene.materials.size(),
		m_Scene.materials.data()
	);

	// prepare entities buffer
	std::vector<ObjectInstance> objects(m_Scene.entities.size());
	for(int32 i = 0; i<m_Scene.entities.size(); ++i)
	{
		Mesh* mesh = m_Scene.meshes[m_Scene.entities[i]->mesh];
		objects[i].params.x = mesh->material;
		objects[i].params.y = m_Scene.entities[i]->mesh;
	}

	m_ObjectsBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(ObjectInstance)*objects.size(),
		objects.data()
	);

	m_Quad = DVKDefaultRes::fullQuad;

	m_StorageImage = DVKTexture::Create2D(
		m_VulkanDevice,
		cmdBuffer,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		ImageLayoutBarrier::ComputeGeneralRW
	);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/68_RTXPathTracing/result.vert.spv",
		"Assets/Shaders/68_RTXPathTracing/result.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();
	m_Material->SetTexture("diffuseMap",m_StorageImage);

	delete cmdBuffer;
}

void RTXPathTracingDemo::LoadExtensions()
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	vkCreateAccelerationStructureNV = reinterpret_cast<PFN_vkCreateAccelerationStructureNV>(vkGetDeviceProcAddr(device,"vkCreateAccelerationStructureNV"));
	vkDestroyAccelerationStructureNV = reinterpret_cast<PFN_vkDestroyAccelerationStructureNV>(vkGetDeviceProcAddr(device,"vkDestroyAccelerationStructureNV"));
	vkBindAccelerationStructureMemoryNV = reinterpret_cast<PFN_vkBindAccelerationStructureMemoryNV>(vkGetDeviceProcAddr(device,"vkBindAccelerationStructureMemoryNV"));
	vkGetAccelerationStructureHandleNV = reinterpret_cast<PFN_vkGetAccelerationStructureHandleNV>(vkGetDeviceProcAddr(device,"vkGetAccelerationStructureHandleNV"));
	vkGetAccelerationStructureMemoryRequirementsNV = reinterpret_cast<PFN_vkGetAccelerationStructureMemoryRequirementsNV>(vkGetDeviceProcAddr(device,"vkGetAccelerationStructureMemoryRequirementsNV"));
	vkCmdBuildAccelerationStructureNV = reinterpret_cast<PFN_vkCmdBuildAccelerationStructureNV>(vkGetDeviceProcAddr(device,"vkCmdBuildAccelerationStructureNV"));
	vkCreateRayTracingPipelinesNV = reinterpret_cast<PFN_vkCreateRayTracingPipelinesNV>(vkGetDeviceProcAddr(device,"vkCreateRayTracingPipelinesNV"));
	vkGetRayTracingShaderGroupHandlesNV = reinterpret_cast<PFN_vkGetRayTracingShaderGroupHandlesNV>(vkGetDeviceProcAddr(device,"vkGetRayTracingShaderGroupHandlesNV"));
	vkCmdTraceRaysNV = reinterpret_cast<PFN_vkCmdTraceRaysNV>(vkGetDeviceProcAddr(device,"vkCmdTraceRaysNV"));

	ZeroVulkanStruct(m_RayTracingPropertiesNV,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PROPERTIES_NV);

	VkPhysicalDeviceProperties2 deviceProperties2;
	ZeroVulkanStruct(deviceProperties2,VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2);
	deviceProperties2.pNext = &m_RayTracingPropertiesNV;
	vkGetPhysicalDeviceProperties2(m_VulkanDevice->GetPhysicalHandle(),&deviceProperties2);
}

void RTXPathTracingDemo::PrepareAS()
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// bottom level
	CreateBottomLevelAS(cmdBuffer);

	// top level
	CreateTopLevelAS(cmdBuffer);

	delete cmdBuffer;
}

void RTXPathTracingDemo::CreateTopLevelAS(DVKCommandBuffer* cmdBuffer)
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	VkAccelerationStructureInfoNV accelerationStructureInfo;
	ZeroVulkanStruct(accelerationStructureInfo,VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV);
	accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_NV;
	accelerationStructureInfo.instanceCount = m_Scene.entities.size();

	VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo;
	ZeroVulkanStruct(accelerationStructureCreateInfo,VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV);
	accelerationStructureCreateInfo.info = accelerationStructureInfo;
	VERIFYVULKANRESULT(vkCreateAccelerationStructureNV(device,&accelerationStructureCreateInfo,VULKAN_CPU_ALLOCATOR,&m_TopLevelAS.accelerationStructure));

	VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
	ZeroVulkanStruct(memoryRequirementsInfo,VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV);
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAS.accelerationStructure;

	VkMemoryRequirements2 memoryRequirements2;
	vkGetAccelerationStructureMemoryRequirementsNV(device,&memoryRequirementsInfo,&memoryRequirements2);

	uint32 memoryTypeIndex = 0;
	m_VulkanDevice->GetMemoryManager()->GetMemoryTypeFromProperties(memoryRequirements2.memoryRequirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,&memoryTypeIndex);

	VkMemoryAllocateInfo memoryAllocateInfo;
	ZeroVulkanStruct(memoryAllocateInfo,VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
	memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
	VERIFYVULKANRESULT(vkAllocateMemory(device,&memoryAllocateInfo,VULKAN_CPU_ALLOCATOR,&m_TopLevelAS.memory));

	VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo;
	ZeroVulkanStruct(accelerationStructureMemoryInfo,VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV);
	accelerationStructureMemoryInfo.accelerationStructure = m_TopLevelAS.accelerationStructure;
	accelerationStructureMemoryInfo.memory = m_TopLevelAS.memory;
	VERIFYVULKANRESULT(vkBindAccelerationStructureMemoryNV(device,1,&accelerationStructureMemoryInfo));

	VERIFYVULKANRESULT(vkGetAccelerationStructureHandleNV(device,m_TopLevelAS.accelerationStructure,sizeof(uint64_t),&m_TopLevelAS.handle));

	// scratch size
	memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
	memoryRequirementsInfo.accelerationStructure = m_TopLevelAS.accelerationStructure;

	VkMemoryRequirements2 topLevelASMemoryRequirements2;
	vkGetAccelerationStructureMemoryRequirementsNV(device,&memoryRequirementsInfo,&topLevelASMemoryRequirements2);

	DVKBuffer* scratchBuffer = DVKBuffer::CreateBuffer(m_VulkanDevice,VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,topLevelASMemoryRequirements2.memoryRequirements.size);

	// geometry instance buffer
	std::vector<VkGeometryInstance> geometryInstances(m_Scene.entities.size());
	for(int32 i = 0; i<m_Scene.entities.size(); ++i)
	{
		Matrix4x4 matrix = m_Scene.entities[i]->GetWorldTransform();
		matrix.SetTransposed();

		VkGeometryInstance& geometryInstance = geometryInstances[i];
		memcpy(geometryInstance.transform,&matrix.m,sizeof(float)*12);

		geometryInstance.instanceId = i;
		geometryInstance.mask = 0xFF;
		geometryInstance.instanceOffset = 0;
		geometryInstance.flags = VK_GEOMETRY_INSTANCE_TRIANGLE_CULL_DISABLE_BIT_NV;
		geometryInstance.accelerationStructureHandle = m_BottomLevelsAS[i].handle;
	}

	DVKBuffer* geometryInstanceBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(VkGeometryInstance)*geometryInstances.size(),
		geometryInstances.data()
	);

	cmdBuffer->Begin();

	vkCmdBuildAccelerationStructureNV(cmdBuffer->cmdBuffer,&accelerationStructureInfo,geometryInstanceBuffer->buffer,0,VK_FALSE,m_TopLevelAS.accelerationStructure,VK_NULL_HANDLE,scratchBuffer->buffer,0);

	cmdBuffer->Submit();

	delete scratchBuffer;
	delete geometryInstanceBuffer;
}

void RTXPathTracingDemo::CreateBottomLevelAS(DVKCommandBuffer* cmdBuffer)
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	struct LocalTransform
	{
		float transform[12];
	};

	// aabb bounds buffer
	{
		std::vector<BoundingBox> aabbBounds(m_Scene.meshes.size());
		for(int32 i = 0; i<aabbBounds.size(); ++i)
		{
			aabbBounds[i] = m_Scene.meshes[i]->bounds;
		}

		DVKBuffer* stagingBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			aabbBounds.size()*sizeof(BoundingBox),
			aabbBounds.data()
		);

		m_ObjectsBoundsBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			aabbBounds.size()*sizeof(BoundingBox)
		);

		cmdBuffer->Begin();

		VkBufferCopy copyRegion = { };
		copyRegion.size = aabbBounds.size()*sizeof(BoundingBox);

		vkCmdCopyBuffer(cmdBuffer->cmdBuffer,stagingBuffer->buffer,m_ObjectsBoundsBuffer->buffer,1,&copyRegion);

		cmdBuffer->End();
		cmdBuffer->Submit();

		delete stagingBuffer;
	}

	// local transform
	{
		std::vector<LocalTransform> transforms(m_Scene.meshes.size());
		for(int32 i = 0; i<transforms.size(); ++i)
		{
			Matrix4x4 matrix;
			// test
			// matrix.AppendTranslation(Vector3(i, 0, 0));
			matrix.SetTransposed();
			memcpy(transforms[i].transform,&matrix.m,sizeof(float)*12);
		}

		DVKBuffer* stagingBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			transforms.size()*sizeof(LocalTransform),
			transforms.data()
		);

		m_ObjectsTransformBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_RAY_TRACING_BIT_NV|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			transforms.size()*sizeof(LocalTransform)
		);

		cmdBuffer->Begin();

		VkBufferCopy copyRegion = { };
		copyRegion.size = transforms.size()*sizeof(LocalTransform);

		vkCmdCopyBuffer(cmdBuffer->cmdBuffer,stagingBuffer->buffer,m_ObjectsTransformBuffer->buffer,1,&copyRegion);

		cmdBuffer->End();
		cmdBuffer->Submit();

		delete stagingBuffer;
	}

	m_BottomLevelsAS.resize(m_Scene.meshes.size());

	for(int32 i = 0; i<m_Scene.meshes.size(); ++i)
	{
		AccelerationStructureInstance& asInstance = m_BottomLevelsAS[i];
		Mesh* mesh = m_Scene.meshes[i];

		VkGeometryNV geometryNV;
		ZeroVulkanStruct(geometryNV,VK_STRUCTURE_TYPE_GEOMETRY_NV);
		geometryNV.flags = VK_GEOMETRY_OPAQUE_BIT_NV;
		geometryNV.geometryType = VK_GEOMETRY_TYPE_TRIANGLES_NV;
		geometryNV.geometry.aabbs = { };
		geometryNV.geometry.aabbs.sType = VK_STRUCTURE_TYPE_GEOMETRY_AABB_NV;
		geometryNV.geometry.aabbs.aabbData = m_ObjectsBoundsBuffer->buffer;
		geometryNV.geometry.aabbs.stride = sizeof(BoundingBox);
		geometryNV.geometry.aabbs.offset = i*sizeof(BoundingBox);
		geometryNV.geometry.triangles.sType = VK_STRUCTURE_TYPE_GEOMETRY_TRIANGLES_NV;
		geometryNV.geometry.triangles.vertexData = mesh->vertexBuffer->buffer;
		geometryNV.geometry.triangles.vertexOffset = 0;
		geometryNV.geometry.triangles.vertexCount = mesh->vertexCount;
		geometryNV.geometry.triangles.vertexStride = mesh->vertexStride;
		geometryNV.geometry.triangles.vertexFormat = VK_FORMAT_R32G32B32_SFLOAT;
		geometryNV.geometry.triangles.indexData = mesh->indexBuffer->buffer;
		geometryNV.geometry.triangles.indexOffset = 0;
		geometryNV.geometry.triangles.indexCount = mesh->indexCount;
		geometryNV.geometry.triangles.indexType = VK_INDEX_TYPE_UINT32;
		geometryNV.geometry.triangles.transformData = m_ObjectsTransformBuffer->buffer;
		geometryNV.geometry.triangles.transformOffset = i*sizeof(LocalTransform);

		VkAccelerationStructureInfoNV accelerationStructureInfo;
		ZeroVulkanStruct(accelerationStructureInfo,VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_INFO_NV);
		accelerationStructureInfo.type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_NV;
		accelerationStructureInfo.geometryCount = 1;
		accelerationStructureInfo.pGeometries = &geometryNV;

		VkAccelerationStructureCreateInfoNV accelerationStructureCreateInfo;
		ZeroVulkanStruct(accelerationStructureCreateInfo,VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_NV);
		accelerationStructureCreateInfo.info = accelerationStructureInfo;
		VERIFYVULKANRESULT(vkCreateAccelerationStructureNV(device,&accelerationStructureCreateInfo,VULKAN_CPU_ALLOCATOR,&asInstance.accelerationStructure));

		VkAccelerationStructureMemoryRequirementsInfoNV memoryRequirementsInfo;
		ZeroVulkanStruct(memoryRequirementsInfo,VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_INFO_NV);
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_OBJECT_NV;
		memoryRequirementsInfo.accelerationStructure = asInstance.accelerationStructure;

		VkMemoryRequirements2 memoryRequirements2;
		vkGetAccelerationStructureMemoryRequirementsNV(device,&memoryRequirementsInfo,&memoryRequirements2);

		uint32 memoryTypeIndex = 0;
		m_VulkanDevice->GetMemoryManager()->GetMemoryTypeFromProperties(memoryRequirements2.memoryRequirements.memoryTypeBits,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,&memoryTypeIndex);

		VkMemoryAllocateInfo memoryAllocateInfo;
		ZeroVulkanStruct(memoryAllocateInfo,VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
		memoryAllocateInfo.allocationSize = memoryRequirements2.memoryRequirements.size;
		memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;
		VERIFYVULKANRESULT(vkAllocateMemory(device,&memoryAllocateInfo,VULKAN_CPU_ALLOCATOR,&asInstance.memory));

		VkBindAccelerationStructureMemoryInfoNV accelerationStructureMemoryInfo;
		ZeroVulkanStruct(accelerationStructureMemoryInfo,VK_STRUCTURE_TYPE_BIND_ACCELERATION_STRUCTURE_MEMORY_INFO_NV);
		accelerationStructureMemoryInfo.accelerationStructure = asInstance.accelerationStructure;
		accelerationStructureMemoryInfo.memory = asInstance.memory;
		VERIFYVULKANRESULT(vkBindAccelerationStructureMemoryNV(device,1,&accelerationStructureMemoryInfo));

		VERIFYVULKANRESULT(vkGetAccelerationStructureHandleNV(device,asInstance.accelerationStructure,sizeof(uint64_t),&asInstance.handle));

		// scratch size
		memoryRequirementsInfo.type = VK_ACCELERATION_STRUCTURE_MEMORY_REQUIREMENTS_TYPE_BUILD_SCRATCH_NV;
		memoryRequirementsInfo.accelerationStructure = asInstance.accelerationStructure;

		VkMemoryRequirements2 bottomLevelASMemoryRequirements2;
		vkGetAccelerationStructureMemoryRequirementsNV(device,&memoryRequirementsInfo,&bottomLevelASMemoryRequirements2);

		DVKBuffer* scratchBuffer = DVKBuffer::CreateBuffer(m_VulkanDevice,VK_BUFFER_USAGE_RAY_TRACING_BIT_NV,VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,bottomLevelASMemoryRequirements2.memoryRequirements.size);

		cmdBuffer->Begin();

		vkCmdBuildAccelerationStructureNV(cmdBuffer->cmdBuffer,&accelerationStructureInfo,VK_NULL_HANDLE,0,VK_FALSE,asInstance.accelerationStructure,VK_NULL_HANDLE,scratchBuffer->buffer,0);

		cmdBuffer->Submit();

		delete scratchBuffer;
	}
}

void RTXPathTracingDemo::DestroyAssets()
{
	VkDevice device = m_VulkanDevice->GetInstanceHandle();

	m_UniformBuffer->UnMap();

	m_Scene.Destroy();

	delete m_Shader;
	delete m_Material;

	delete m_LightsBuffer;
	delete m_MaterialsBuffer;
	delete m_ObjectsBuffer;
	delete m_ObjectsBoundsBuffer;
	delete m_ObjectsTransformBuffer;
	delete m_StorageImage;
	delete m_UniformBuffer;
	delete m_ShaderBindingTable;

	for(int32 i = 0; i<m_BottomLevelsAS.size(); ++i)
	{
		vkDestroyAccelerationStructureNV(device,m_BottomLevelsAS[i].accelerationStructure,VULKAN_CPU_ALLOCATOR);
		vkFreeMemory(device,m_BottomLevelsAS[i].memory,VULKAN_CPU_ALLOCATOR);
	}

	vkDestroyAccelerationStructureNV(device,m_TopLevelAS.accelerationStructure,VULKAN_CPU_ALLOCATOR);
	vkFreeMemory(device,m_TopLevelAS.memory,VULKAN_CPU_ALLOCATOR);

	vkDestroyPipeline(device,m_Pipeline,VULKAN_CPU_ALLOCATOR);
	vkDestroyPipelineLayout(device,m_PipelineLayout,VULKAN_CPU_ALLOCATOR);

	for(int32 i = 0; i<m_DescriptorSetLayouts.size(); ++i)
	{
		vkDestroyDescriptorSetLayout(device,m_DescriptorSetLayouts[i],VULKAN_CPU_ALLOCATOR);
	}

	vkDestroyDescriptorPool(device,m_DescriptorPool,VULKAN_CPU_ALLOCATOR);
}

void RTXPathTracingDemo::SetupGfxCommand(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	// raytracing
	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,m_Pipeline);
	vkCmdBindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_RAY_TRACING_NV,m_PipelineLayout,0,m_DescriptorSets.size(),m_DescriptorSets.data(),0,0);

	VkDeviceSize stride = Align(m_RayTracingPropertiesNV.shaderGroupHandleSize,m_RayTracingPropertiesNV.shaderGroupBaseAlignment);

	vkCmdTraceRaysNV(
		commandBuffer,
		m_ShaderBindingTable->buffer,
		stride*0,
		m_ShaderBindingTable->buffer,
		stride*1,
		stride,
		m_ShaderBindingTable->buffer,
		stride*2,
		stride,
		VK_NULL_HANDLE,
		0,
		0,
		m_FrameWidth,
		m_FrameHeight,
		1
	);

	// postprocess pass
	VkViewport viewport = { };
	viewport.x = 0;
	viewport.y = m_FrameHeight;
	viewport.width = m_FrameWidth;
	viewport.height = -m_FrameHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.extent.width = m_FrameWidth;
	scissor.extent.height = m_FrameHeight;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	VkClearValue clearValues[2];
	clearValues[0].color = {
		{ 0.0f,0.0f,0.0f,1.0f }
	};
	clearValues[1].depthStencil = { 1.0f,0 };

	VkRenderPassBeginInfo renderPassBeginInfo;
	ZeroVulkanStruct(renderPassBeginInfo,VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.framebuffer = m_FrameBuffers[backBufferIndex];
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_FrameWidth;
	renderPassBeginInfo.renderArea.extent.height = m_FrameHeight;
	vkCmdBeginRenderPass(commandBuffer,&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(commandBuffer,0,1,&viewport);
	vkCmdSetScissor(commandBuffer,0,1,&scissor);

	m_Material->BeginFrame();
	m_Material->BeginObject();
	m_Material->SetLocalUniform("uboParam",&m_GlobalParam.moving,sizeof(Vector4));
	m_Material->EndObject();
	m_Material->EndFrame();
	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());
	m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

	// ui pass
	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void RTXPathTracingDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void RTXPathTracingDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}
