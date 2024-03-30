#include "LoadMesh.h"
#include "Math/Matrix4x4.h"
#include <Source/Graphics/DVKUtils.h>

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<LoadMesh>(1400,900,"LoadMesh",cmdLine);
}

LoadMesh::LoadMesh(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine)
	:ModuleBase(width,height,title,cmdLine)
{
}

LoadMesh::~LoadMesh()
{
}

bool LoadMesh::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();
	LoadAssets();
	CreateGUI();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();
	CreateDescriptorSet();
	CreatePipelines();
	SetupCommandBuffers();
	m_Ready = true;
	return true;
}

void LoadMesh::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void LoadMesh::Draw(float time,float delta)
{
	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	UpdateUniformBuffers(time,delta);

	int32 bufferIndex = AcquireBackbufferIndex();
	Present(bufferIndex);
}

bool LoadMesh::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();
	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("LoadMesh!",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
		ImGui::Text("Load mesh from file.");

		ImGui::Checkbox("AutoRotate",&m_AutoRotate);

		for(int32 i = 0; i<m_Model->meshes.size(); ++i)
		{
			DVKMesh* mesh = m_Model->meshes[i];
			ImGui::Text("%-20s Tri:%d",mesh->linkNode->name.c_str(),mesh->triangleCount);
		}

		ImGui::Text("%.3f ms/frame (%.1f FPS)",1000.0f/ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();

	if(m_GUI->Update())
	{
		SetupCommandBuffers();
	}

	return hovered;
}

void LoadMesh::UpdateUniformBuffers(float time,float delta)
{
	for(int32 i = 0; i<m_MVPDatas.size(); ++i)
	{
		if(m_AutoRotate)
		{
			m_MVPDatas[i].model.AppendRotation(90.0f*delta,Vector3::UpVector);
		}

		m_MVPDatas[i].view = m_ViewCamera.GetView();
		m_MVPDatas[i].projection = m_ViewCamera.GetProjection();

		m_MVPBuffers[i]->CopyFrom(&(m_MVPDatas[i]),sizeof(UBOData));
	}
}

void LoadMesh::Exist()
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
	DestroyDescriptorSetLayout();
	DestroyDescriptorPool();
	DestroyPipelines();
	DestroyUniformBuffers();
}

void LoadMesh::LoadAssets()
{
	auto cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);
	m_Model = DVKModel::LoadFromFile("Assets/models/suzanne.obj",m_VulkanDevice,cmdBuffer,{ VertexAttribute::VA_Position,VertexAttribute::VA_Normal });
	delete cmdBuffer;
}

void LoadMesh::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void LoadMesh::CreateUniformBuffers()
{
	DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;

	m_MVPDatas.resize(m_Model->meshes.size());
	m_MVPBuffers.resize(m_Model->meshes.size());

	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		m_MVPDatas[i].model.AppendRotation(180,Vector3::UpVector);

		m_MVPBuffers[i] = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			sizeof(UBOData),
			&(m_MVPDatas[i])
		);
		m_MVPBuffers[i]->Map();
	}

	m_ViewCamera.Perspective(PI/4,GetWidth(),GetHeight(),0.1f,1000.0f);
	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y,boundCenter.z-50.0f);
	m_ViewCamera.LookAt(boundCenter);
}

void LoadMesh::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize = { };
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorPoolInfo;
	ZeroVulkanStruct(descriptorPoolInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = &poolSize;
	descriptorPoolInfo.maxSets = m_Model->meshes.size();
	VERIFYVULKANRESULT(vkCreateDescriptorPool(m_Device,&descriptorPoolInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorPool));
}

void LoadMesh::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBinding.descriptorCount = 1;
	layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBinding.pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descSetLayoutInfo;
	ZeroVulkanStruct(descSetLayoutInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
	descSetLayoutInfo.bindingCount = 1;
	descSetLayoutInfo.pBindings = &layoutBinding;
	VERIFYVULKANRESULT(vkCreateDescriptorSetLayout(m_Device,&descSetLayoutInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorSetLayout));

	VkPipelineLayoutCreateInfo pipeLayoutInfo;
	ZeroVulkanStruct(pipeLayoutInfo,VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
	pipeLayoutInfo.setLayoutCount = 1;
	pipeLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
	VERIFYVULKANRESULT(vkCreatePipelineLayout(m_Device,&pipeLayoutInfo,VULKAN_CPU_ALLOCATOR,&m_PipelineLayout));
}

void LoadMesh::CreateDescriptorSet()
{
	m_DescriptorSets.resize(m_Model->meshes.size());
	for(int32 i = 0; i<m_DescriptorSets.size(); ++i)
	{
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		VkDescriptorSetAllocateInfo allocInfo;
		ZeroVulkanStruct(allocInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
		allocInfo.descriptorPool = m_DescriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &m_DescriptorSetLayout;
		VERIFYVULKANRESULT(vkAllocateDescriptorSets(m_Device,&allocInfo,&descriptorSet));

		VkWriteDescriptorSet writeDescriptorSet;
		ZeroVulkanStruct(writeDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.pBufferInfo = &(m_MVPBuffers[i]->descriptor);
		writeDescriptorSet.dstBinding = 0;
		vkUpdateDescriptorSets(m_Device,1,&writeDescriptorSet,0,nullptr);
		m_DescriptorSets[i] = descriptorSet;
	}
}

void LoadMesh::CreatePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	ZeroVulkanStruct(inputAssemblyState,VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	VkPipelineRasterizationStateCreateInfo rasterizationState;
	ZeroVulkanStruct(rasterizationState,VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizationState.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = { };
	blendAttachmentState[0].colorWriteMask = (
		VK_COLOR_COMPONENT_R_BIT|
		VK_COLOR_COMPONENT_G_BIT|
		VK_COLOR_COMPONENT_B_BIT|
		VK_COLOR_COMPONENT_A_BIT
		);
	blendAttachmentState[0].blendEnable = VK_FALSE;
	blendAttachmentState[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState[0].colorBlendOp = VK_BLEND_OP_ADD;
	blendAttachmentState[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	blendAttachmentState[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	blendAttachmentState[0].alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendState;
	ZeroVulkanStruct(colorBlendState,VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

	VkPipelineViewportStateCreateInfo viewportState;
	ZeroVulkanStruct(viewportState,VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);

	VkPipelineDynamicStateCreateInfo dynamicState;
	ZeroVulkanStruct(dynamicState,VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStateEnables.data();

	VkPipelineDepthStencilStateCreateInfo depthStencilState;
	ZeroVulkanStruct(depthStencilState,VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
	depthStencilState.depthTestEnable = VK_TRUE;
	depthStencilState.depthWriteEnable = VK_TRUE;
	depthStencilState.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilState.depthBoundsTestEnable = VK_FALSE;
	depthStencilState.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilState.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilState.stencilTestEnable = VK_FALSE;
	depthStencilState.front = depthStencilState.back;

	VkPipelineMultisampleStateCreateInfo multisampleState;
	ZeroVulkanStruct(multisampleState,VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
	multisampleState.rasterizationSamples = m_SampleCount;
	multisampleState.pSampleMask = nullptr;

	VkVertexInputBindingDescription vertexInputBinding = m_Model->GetInputBinding();
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributs = m_Model->GetInputAttributes();

	VkPipelineVertexInputStateCreateInfo vertexInputState;
	ZeroVulkanStruct(vertexInputState,VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputState.vertexAttributeDescriptionCount = m_Model->attributes.size();
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
	ZeroVulkanStruct(shaderStages[0],VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
	ZeroVulkanStruct(shaderStages[1],VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = LoadSPIPVShader(m_Device,"Assets/Shaders/6_LoadMesh/mesh.vert.spv");
	shaderStages[0].pName = "main";
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = LoadSPIPVShader(m_Device,"Assets/Shaders/6_LoadMesh/mesh.frag.spv");
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	ZeroVulkanStruct(pipelineCreateInfo,VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
	pipelineCreateInfo.layout = m_PipelineLayout;
	pipelineCreateInfo.renderPass = m_RenderPass;
	pipelineCreateInfo.stageCount = shaderStages.size();
	pipelineCreateInfo.pStages = shaderStages.data();
	pipelineCreateInfo.pVertexInputState = &vertexInputState;
	pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
	pipelineCreateInfo.pRasterizationState = &rasterizationState;
	pipelineCreateInfo.pColorBlendState = &colorBlendState;
	pipelineCreateInfo.pMultisampleState = &multisampleState;
	pipelineCreateInfo.pViewportState = &viewportState;
	pipelineCreateInfo.pDepthStencilState = &depthStencilState;
	pipelineCreateInfo.pDynamicState = &dynamicState;
	VERIFYVULKANRESULT(vkCreateGraphicsPipelines(m_Device,m_PipelineCache,1,&pipelineCreateInfo,VULKAN_CPU_ALLOCATOR,&m_Pipeline));

	vkDestroyShaderModule(m_Device,shaderStages[0].module,VULKAN_CPU_ALLOCATOR);
	vkDestroyShaderModule(m_Device,shaderStages[1].module,VULKAN_CPU_ALLOCATOR);
}

void LoadMesh::SetupCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

	VkClearValue clearValues[2];
	clearValues[0].color = {
		{ 0.2f,0.2f,0.2f,1.0f }
	};
	clearValues[1].depthStencil = { 1.0f,0 };

	VkRenderPassBeginInfo renderPassBeginInfo;
	ZeroVulkanStruct(renderPassBeginInfo,VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.clearValueCount = 2;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_FrameWidth;
	renderPassBeginInfo.renderArea.extent.height = m_FrameHeight;

	for(int32 i = 0; i<m_CommandBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = m_FrameBuffers[i];

		VkViewport viewport = { };
		viewport.x = 0;
		viewport.y = m_FrameHeight;
		viewport.width = (float)m_FrameWidth;
		viewport.height = -(float)m_FrameHeight;    // flip y axis
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = { };
		scissor.extent.width = m_FrameWidth;
		scissor.extent.height = m_FrameHeight;
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		VERIFYVULKANRESULT(vkBeginCommandBuffer(m_CommandBuffers[i],&cmdBeginInfo));

		vkCmdBeginRenderPass(m_CommandBuffers[i],&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(m_CommandBuffers[i],0,1,&viewport);
		vkCmdSetScissor(m_CommandBuffers[i],0,1,&scissor);

		vkCmdBindPipeline(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline);

		for(int32 meshIndex = 0; meshIndex<m_Model->meshes.size(); ++meshIndex)
		{
			vkCmdBindDescriptorSets(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_PipelineLayout,0,1,&m_DescriptorSets[meshIndex],0,nullptr);
			m_Model->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);
		}

		m_GUI->BindDrawCmd(m_CommandBuffers[i],m_RenderPass);

		vkCmdEndRenderPass(m_CommandBuffers[i]);

		VERIFYVULKANRESULT(vkEndCommandBuffer(m_CommandBuffers[i]));
	}
}

void LoadMesh::DestroyAssets()
{
	delete m_Model;
	m_Model = nullptr;
}

void LoadMesh::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
	m_GUI = nullptr;
}

void LoadMesh::DestroyDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_Device,m_DescriptorSetLayout,VULKAN_CPU_ALLOCATOR);
	vkDestroyPipelineLayout(m_Device,m_PipelineLayout,VULKAN_CPU_ALLOCATOR);
}

void LoadMesh::DestroyDescriptorPool()
{
	vkDestroyDescriptorPool(m_Device,m_DescriptorPool,VULKAN_CPU_ALLOCATOR);
}

void LoadMesh::DestroyPipelines()
{
	vkDestroyPipeline(m_Device,m_Pipeline,VULKAN_CPU_ALLOCATOR);
}

void LoadMesh::DestroyUniformBuffers()
{
	m_MVPDatas.clear();

	for(int32 i = 0; i<m_MVPBuffers.size(); ++i)
	{
		DVKBuffer* buffer = m_MVPBuffers[i];
		buffer->UnMap();
		delete buffer;
	}

	m_MVPBuffers.clear();
}

