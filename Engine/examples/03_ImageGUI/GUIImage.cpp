#include "GUIImage.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<GUIImage>(1400,900,"GUI",cmdLine);
}

GUIImage::GUIImage(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :ModuleBase(width,height,title,cmdLine)
{
}

GUIImage::~GUIImage()
{
}

bool GUIImage::PreInit()
{
	return true;
}

bool GUIImage::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();
	CreateGUI();
	CreateMeshBuffers();
	CreateUniformBuffers();
	CreateDescriptorPool();
	CreateDescriptorSetLayout();
	CreateDescriptorSet();
	CreatePipelines();
	SetupCommandBuffers();
	m_Ready = true;
	return true;
}

void GUIImage::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void GUIImage::Exist()
{
	ModuleBase::Release();
	DestroyGUI();
	DestroyMeshBuffers();
	DestroyUniformBuffers();
	DestroyDescriptorPool();
	DestroyDescriptorSetLayout();
	DestroyPipelines();
}

void GUIImage::Draw(float time,float delta)
{
	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}
	UpdateUniformBuffers(time,delta);
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();
	ModuleBase::Present(bufferIndex);
}

bool GUIImage::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();
	bool yes = true;

	static float f = 0.0f;
	static Vector3 color(0,0,0);
	static int counter = 0;

	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
	ImGui::Begin("ImGUI!",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
	ImGui::Checkbox("AutoRotate",&m_EnableRotate);
	ImGui::Text("This is some useful text.");
	ImGui::Checkbox("Demo Window",&yes);
	ImGui::Checkbox("Another Window",&yes);
	ImGui::SliderFloat("float",&f,0.0f,1.0f);
	ImGui::ColorEdit3("clear color",(float*)&color);
	if(ImGui::Button("Button"))
	{
		counter++;
	}
	ImGui::SameLine();
	ImGui::Text("counter = %d",counter);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",1000.0f/ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
	ImGui::End();
	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();
	m_GUI->EndFrame();

	if(m_GUI->Update())
	{
		SetupCommandBuffers();
	}
	return hovered;
}

void GUIImage::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void GUIImage::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}

void GUIImage::CreateDescriptorPool()
{
	VkDescriptorPoolSize poolSize = { };
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 1;

	VkDescriptorPoolCreateInfo descriptorPoolInfo;
	ZeroVulkanStruct(descriptorPoolInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = &poolSize;
	descriptorPoolInfo.maxSets = 1;
	VERIFYVULKANRESULT(vkCreateDescriptorPool(m_Device,&descriptorPoolInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorPool));
}

void GUIImage::DestroyDescriptorPool()
{
	vkDestroyDescriptorPool(m_Device,m_DescriptorPool,VULKAN_CPU_ALLOCATOR);
}

void GUIImage::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBinding;
	layoutBinding.binding = 0;
	layoutBinding.descriptorCount = 1;
	layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
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

void GUIImage::DestroyDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_Device,m_DescriptorSetLayout,VULKAN_CPU_ALLOCATOR);
	vkDestroyPipelineLayout(m_Device,m_PipelineLayout,VULKAN_CPU_ALLOCATOR);
}

void GUIImage::CreateDescriptorSet()
{
	VkDescriptorSetAllocateInfo allocInfo;
	ZeroVulkanStruct(allocInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
	allocInfo.descriptorPool = m_DescriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = &m_DescriptorSetLayout;
	VERIFYVULKANRESULT(vkAllocateDescriptorSets(m_Device,&allocInfo,&m_DescriptorSet));

	VkWriteDescriptorSet writeDescriptorSet;
	ZeroVulkanStruct(writeDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	writeDescriptorSet.dstSet = m_DescriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &(m_MVPBuffer->descriptor);
	writeDescriptorSet.dstBinding = 0;
	vkUpdateDescriptorSets(m_Device,1,&writeDescriptorSet,0,nullptr);
}

void GUIImage::CreatePipelines()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyState;
	ZeroVulkanStruct(inputAssemblyState,VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
	inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	//光栅化视口剪裁
	VkPipelineRasterizationStateCreateInfo rasterizationState;
	ZeroVulkanStruct(rasterizationState,VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
	rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizationState.cullMode = VK_CULL_MODE_NONE;
	rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizationState.depthClampEnable = VK_FALSE;
	rasterizationState.rasterizerDiscardEnable = VK_FALSE;
	rasterizationState.depthBiasEnable = VK_FALSE;
	rasterizationState.lineWidth = 1.0f;

	//颜色混合
	VkPipelineColorBlendAttachmentState blendAttachmentState[1] = { };
	blendAttachmentState[0].colorWriteMask = (
		VK_COLOR_COMPONENT_R_BIT|
		VK_COLOR_COMPONENT_G_BIT|
		VK_COLOR_COMPONENT_B_BIT|
		VK_COLOR_COMPONENT_A_BIT
		);
	blendAttachmentState[0].blendEnable = VK_FALSE;

	VkPipelineColorBlendStateCreateInfo colorBlendState;
	ZeroVulkanStruct(colorBlendState,VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
	colorBlendState.attachmentCount = 1;
	colorBlendState.pAttachments = blendAttachmentState;

	//视口设置
	VkPipelineViewportStateCreateInfo viewportState;
	ZeroVulkanStruct(viewportState,VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
	viewportState.viewportCount = 1;
	viewportState.scissorCount = 1;

	//动态状态,视口和剪裁大小
	std::vector<VkDynamicState> dynamicStateEnables;
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);
	dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);
	VkPipelineDynamicStateCreateInfo dynamicState;
	ZeroVulkanStruct(dynamicState,VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
	dynamicState.dynamicStateCount = (uint32_t)dynamicStateEnables.size();
	dynamicState.pDynamicStates = dynamicStateEnables.data();

	//深度模板检测
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

	//抗锯齿
	VkPipelineMultisampleStateCreateInfo multisampleState;
	ZeroVulkanStruct(multisampleState,VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
	multisampleState.rasterizationSamples = m_SampleCount;
	multisampleState.pSampleMask = nullptr;

	// (triangle.vert):
	// layout (location = 0) in vec3 inPos;
	// layout (location = 1) in vec3 inColor;
	// Attribute location 0: Position
	// Attribute location 1: Color
	// vertex input bindding
	VkVertexInputBindingDescription vertexInputBinding = { };
	vertexInputBinding.binding = 0; // Vertex Buffer 0
	vertexInputBinding.stride = sizeof(Vertex); // Position + Color
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	std::vector<VkVertexInputAttributeDescription> vertexInputAttributs(2);
	// position
	vertexInputAttributs[0].binding = 0;
	vertexInputAttributs[0].location = 0; // triangle.vert : layout (location = 0)
	vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs[0].offset = 0;
	// color
	vertexInputAttributs[1].binding = 0;
	vertexInputAttributs[1].location = 1; // triangle.vert : layout (location = 1)
	vertexInputAttributs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	vertexInputAttributs[1].offset = 12;

	VkPipelineVertexInputStateCreateInfo vertexInputState;
	ZeroVulkanStruct(vertexInputState,VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
	vertexInputState.vertexBindingDescriptionCount = 1;
	vertexInputState.pVertexBindingDescriptions = &vertexInputBinding;
	vertexInputState.vertexAttributeDescriptionCount = 2;
	vertexInputState.pVertexAttributeDescriptions = vertexInputAttributs.data();

	std::vector<VkPipelineShaderStageCreateInfo> shaderStages(2);
	ZeroVulkanStruct(shaderStages[0],VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
	ZeroVulkanStruct(shaderStages[1],VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = LoadSPIPVShader(m_Device,"Assets/Shaders/1_Triangle/triangle.vert.spv");
	shaderStages[0].pName = "main";
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = LoadSPIPVShader(m_Device,"Assets/Shaders/1_Triangle/triangle.frag.spv");
	shaderStages[1].pName = "main";

	VkGraphicsPipelineCreateInfo pipelineCreateInfo;
	ZeroVulkanStruct(pipelineCreateInfo,VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
	pipelineCreateInfo.layout = m_PipelineLayout;
	pipelineCreateInfo.renderPass = m_RenderPass;
	pipelineCreateInfo.stageCount = (uint32_t)shaderStages.size();
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
}

void GUIImage::DestroyPipelines()
{
	vkDestroyPipeline(m_Device,m_Pipeline,VULKAN_CPU_ALLOCATOR);
}

void GUIImage::SetupCommandBuffers()
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

		VkDeviceSize offsets[1] = { 0 };

		VERIFYVULKANRESULT(vkBeginCommandBuffer(m_CommandBuffers[i],&cmdBeginInfo));

		vkCmdBeginRenderPass(m_CommandBuffers[i],&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);
		vkCmdSetViewport(m_CommandBuffers[i],0,1,&viewport);
		vkCmdSetScissor(m_CommandBuffers[i],0,1,&scissor);

		vkCmdBindDescriptorSets(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_PipelineLayout,0,1,&m_DescriptorSet,0,nullptr);
		vkCmdBindPipeline(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline);
		vkCmdBindVertexBuffers(m_CommandBuffers[i],0,1,&(m_VertexBuffer->buffer),offsets);
		vkCmdBindIndexBuffer(m_CommandBuffers[i],m_IndexBuffer->buffer,0,VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(m_CommandBuffers[i],m_IndicesCount,1,0,0,0);

		m_GUI->BindDrawCmd(m_CommandBuffers[i],m_RenderPass);
		vkCmdEndRenderPass(m_CommandBuffers[i]);
		VERIFYVULKANRESULT(vkEndCommandBuffer(m_CommandBuffers[i]));
	}
}

void GUIImage::UpdateUniformBuffers(float time,float delta)
{
	if(m_EnableRotate)
	{
		m_MVPData.model.AppendRotation(90.0f*delta,Vector3::UpVector);
	}
	m_MVPData.view = m_ViewCamera.GetView();
	m_MVPData.projection = m_ViewCamera.GetProjection();
	m_MVPBuffer->CopyFrom(&m_MVPData,sizeof(UBOData));
}

void GUIImage::CreateMeshBuffers()
{
	std::vector<Vertex> vertices =
	{
		{
			{ 1.0,1.0,0.0,},{ 1.0,0.0,0.0 }
		},
		{
			{ -1.0,1.0,0.0 },{ 0.0,1.0,0.0 }
		},
		{
			{ 0.0,-1.0,0.0 },{ 0.0,0.0,1.0 }
		}
	};
	std::vector<uint16> indices = { 0,1,2 };
	m_IndicesCount = indices.size();

	auto verstaging = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertices.size()*sizeof(Vertex),
		vertices.data());

	auto idexStaging = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		indices.size()*sizeof(uint16),
		indices.data()
	);

	m_VertexBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertices.size()*sizeof(Vertex)
	);
	m_IndexBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_INDEX_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indices.size()*sizeof(uint16)
	);
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);
	cmdBuffer->Begin();
	VkBufferCopy copyRegion = { };
	copyRegion.size = vertices.size()*sizeof(Vertex);
	vkCmdCopyBuffer(cmdBuffer->cmdBuffer,verstaging->buffer,m_VertexBuffer->buffer,1,&copyRegion);

	copyRegion.size = indices.size()*sizeof(uint16);
	vkCmdCopyBuffer(cmdBuffer->cmdBuffer,idexStaging->buffer,m_IndexBuffer->buffer,1,&copyRegion);
	cmdBuffer->End();
	cmdBuffer->Submit();

	delete cmdBuffer;
	delete verstaging;
	delete idexStaging;
}

void GUIImage::DestroyMeshBuffers()
{
	delete m_VertexBuffer;
	delete m_IndexBuffer;
}

void GUIImage::CreateUniformBuffers()
{
	m_ViewCamera.Perspective(PI/4.0,GetWidth(),GetHeight(),0.1f,1000.0f);
	m_ViewCamera.SetPosition(0,0,-5.0f);
	m_ViewCamera.LookAt(0,0,0);

	m_MVPBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(UBOData),
		&m_MVPData
	);
	m_MVPBuffer->Map();
}

void GUIImage::DestroyUniformBuffers()
{
	m_MVPBuffer->UnMap();
	delete m_MVPBuffer;
}

