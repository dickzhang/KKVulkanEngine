#include "Pipelines.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<PipeLines>(1400,900,"PipeLines",cmdLine);
}

PipeLines::PipeLines(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine)
	:ModuleBase(width,height,title,cmdLine)
{

}

PipeLines::~PipeLines()
{

}

bool PipeLines::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();
	LoadAssets();
	CreateGUI();
	CreateUniformBuffers();
	CreateDescriptorSetLayout();
	CreateDescriptorSet();
	CreatePipelines();
	SetupCommandBuffers();
	m_Ready = true;
	return true;
}

void PipeLines::Exist()
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
	DestroyDescriptorSetLayout();
	DestroyPipelines();
	DestroyUniformBuffers();
}

void PipeLines::Loop(float time,float delta)
{
	if(!m_Ready)return;
	Draw(time,delta);
}

void PipeLines::Draw(float time,float delta)
{
	auto bufferindex = AcquireBackbufferIndex();
	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}
	UpdateUniformBuffers(time,delta);
	Present(bufferindex);
}

bool PipeLines::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();
	ImGui::SetNextWindowPos(ImVec2(0,0));
	ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
	ImGui::Begin("PipeLines",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
	ImGui::Checkbox("AutoRotate",&m_AutoRotate);
	ImGui::SliderFloat("Intensity",&(m_ParamData.intensity),0.0f,1.0f);
	ImGui::Text("%.3f ms/frame (%.1f FPS)",1000.0f/ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
	ImGui::End();
	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();
	m_GUI->EndFrame();
	if(m_GUI->Update())
	{
		SetupCommandBuffers();
	}
	return hovered;
}

void PipeLines::UpdateUniformBuffers(float time,float delta)
{
	if(m_AutoRotate)
	{
		m_MVPData.model.AppendRotation(90.0f*delta,Vector3::UpVector);
	}
	m_MVPData.view = m_ViewCamera.GetView();
	m_MVPData.projection = m_ViewCamera.GetProjection();

	m_MVPBuffer->CopyFrom(&m_MVPData,sizeof(MVPBlock));
	m_ParamBuffer->CopyFrom(&m_ParamData,sizeof(ParamBlock));
}

void PipeLines::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void PipeLines::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
	m_GUI = nullptr;
}

void PipeLines::LoadAssets()
{
	auto cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);
	m_Model = DVKModel::LoadFromFile("Assets/models/suzanne.obj",m_VulkanDevice,cmdBuffer,{ VertexAttribute::VA_Position,VertexAttribute::VA_Normal });
	delete cmdBuffer;
}

void PipeLines::DestroyAssets()
{
	KK_DELETE(m_Model,DVKModel);
}

void PipeLines::CreateUniformBuffers()
{
	auto bounds = m_Model->rootNode->GetBounds();
	Vector3 boundsize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundsize*0.5;

	m_MVPData.model.AppendRotation(180,Vector3::UpVector);

	m_ViewCamera.Perspective(PI/4,GetWidth()/3.0,GetHeight(),0.1,1000.f);
	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y,boundCenter.z-boundsize.Size()*2.0);
	m_ViewCamera.LookAt(boundCenter);

	m_MVPBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(MVPBlock),
		&(m_MVPData)
	);
	m_MVPBuffer->Map();

	m_ParamData.intensity = 0.125f;
	m_ParamBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(ParamBlock),
		&(m_ParamData)
	);
	m_ParamBuffer->Map();
}

void PipeLines::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding layoutBindings[2] = { };
	layoutBindings[0].binding = 0;
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[0].pImmutableSamplers = nullptr;

	layoutBindings[1].binding = 1;
	layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[1].descriptorCount = 1;
	layoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[1].pImmutableSamplers = nullptr;

	VkDescriptorSetLayoutCreateInfo descSetLayoutInfo;
	ZeroVulkanStruct(descSetLayoutInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
	descSetLayoutInfo.bindingCount = 2;
	descSetLayoutInfo.pBindings = layoutBindings;
	VERIFYVULKANRESULT(vkCreateDescriptorSetLayout(m_Device,&descSetLayoutInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorSetLayout));

	VkPipelineLayoutCreateInfo pipeLayoutInfo;
	ZeroVulkanStruct(pipeLayoutInfo,VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
	pipeLayoutInfo.setLayoutCount = 1;
	pipeLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;
	VERIFYVULKANRESULT(vkCreatePipelineLayout(m_Device,&pipeLayoutInfo,VULKAN_CPU_ALLOCATOR,&m_PipelineLayout));
}
void PipeLines::SetupCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

	VkClearValue clearValues[2];
	clearValues[0].color =
	{
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

		VERIFYVULKANRESULT(vkBeginCommandBuffer(m_CommandBuffers[i],&cmdBeginInfo));
		vkCmdBeginRenderPass(m_CommandBuffers[i],&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

		for(int32 j = 0; j<3; ++j)
		{
			int32 ww = 1.0f/3*m_FrameWidth;
			int32 tx = j*ww;

			VkViewport viewport = { };
			viewport.x = tx;
			viewport.y = m_FrameHeight;
			viewport.width = ww;
			viewport.height = -(float)m_FrameHeight;    // flip y axis
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			VkRect2D scissor = { };
			scissor.extent.width = ww;
			scissor.extent.height = m_FrameHeight;
			scissor.offset.x = tx;
			scissor.offset.y = 0;

			vkCmdSetViewport(m_CommandBuffers[i],0,1,&viewport);
			vkCmdSetScissor(m_CommandBuffers[i],0,1,&scissor);

			vkCmdBindPipeline(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipelines[j]->pipeline);
			vkCmdBindDescriptorSets(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipelines[j]->pipelineLayout,0,1,&m_DescriptorSet,0,nullptr);

			for(int32 meshIndex = 0; meshIndex<m_Model->meshes.size(); ++meshIndex)
			{
				m_Model->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);
			}
		}

		m_GUI->BindDrawCmd(m_CommandBuffers[i],m_RenderPass);

		vkCmdEndRenderPass(m_CommandBuffers[i]);
		VERIFYVULKANRESULT(vkEndCommandBuffer(m_CommandBuffers[i]));
	}
}

void PipeLines::DestroyDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(m_Device,m_DescriptorSetLayout,VULKAN_CPU_ALLOCATOR);
	vkDestroyPipelineLayout(m_Device,m_PipelineLayout,VULKAN_CPU_ALLOCATOR);
	vkDestroyDescriptorPool(m_Device,m_DescriptorPool,VULKAN_CPU_ALLOCATOR);
}
void PipeLines::CreateDescriptorSet()
{
	VkDescriptorPoolSize poolSize = { };
	poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSize.descriptorCount = 2;

	VkDescriptorPoolCreateInfo descriptorPoolInfo;
	ZeroVulkanStruct(descriptorPoolInfo,VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
	descriptorPoolInfo.poolSizeCount = 1;
	descriptorPoolInfo.pPoolSizes = &poolSize;
	descriptorPoolInfo.maxSets = m_Model->meshes.size();
	VERIFYVULKANRESULT(vkCreateDescriptorPool(m_Device,&descriptorPoolInfo,VULKAN_CPU_ALLOCATOR,&m_DescriptorPool));

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

	ZeroVulkanStruct(writeDescriptorSet,VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
	writeDescriptorSet.dstSet = m_DescriptorSet;
	writeDescriptorSet.descriptorCount = 1;
	writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writeDescriptorSet.pBufferInfo = &(m_ParamBuffer->descriptor);
	writeDescriptorSet.dstBinding = 1;
	vkUpdateDescriptorSets(m_Device,1,&writeDescriptorSet,0,nullptr);
}

void PipeLines::CreatePipelines()
{
	m_Pipelines.resize(3);

	VkVertexInputBindingDescription vertexInputBinding = m_Model->GetInputBinding();
	std::vector<VkVertexInputAttributeDescription> vertexInputAttributs = m_Model->GetInputAttributes();

	DVKGfxPipelineInfo pipelineInfo0;
	pipelineInfo0.vertShaderModule = LoadSPIPVShader(m_Device,"Assets/Shaders/7_Pipelines/pipeline0.vert.spv");
	pipelineInfo0.fragShaderModule = LoadSPIPVShader(m_Device,"Assets/Shaders/7_Pipelines/pipeline0.frag.spv");

	m_Pipelines[0] = DVKGfxPipeline::Create(m_VulkanDevice,m_PipelineCache,pipelineInfo0,{ vertexInputBinding },vertexInputAttributs,m_PipelineLayout,m_RenderPass);

	DVKGfxPipelineInfo pipelineInfo1;
	pipelineInfo1.vertShaderModule = LoadSPIPVShader(m_Device,"Assets/Shaders/7_Pipelines/pipeline1.vert.spv");
	pipelineInfo1.fragShaderModule = LoadSPIPVShader(m_Device,"Assets/Shaders/7_Pipelines/pipeline1.frag.spv");

	m_Pipelines[1] = DVKGfxPipeline::Create(m_VulkanDevice,m_PipelineCache,pipelineInfo1,{ vertexInputBinding },vertexInputAttributs,m_PipelineLayout,m_RenderPass);

	DVKGfxPipelineInfo pipelineInfo2;
	pipelineInfo2.vertShaderModule = LoadSPIPVShader(m_Device,"Assets/Shaders/7_Pipelines/pipeline2.vert.spv");
	pipelineInfo2.fragShaderModule = LoadSPIPVShader(m_Device,"Assets/Shaders/7_Pipelines/pipeline2.frag.spv");

	m_Pipelines[2] = DVKGfxPipeline::Create(m_VulkanDevice,m_PipelineCache,pipelineInfo2,{ vertexInputBinding },vertexInputAttributs,m_PipelineLayout,m_RenderPass);

	vkDestroyShaderModule(m_Device,pipelineInfo0.vertShaderModule,VULKAN_CPU_ALLOCATOR);
	vkDestroyShaderModule(m_Device,pipelineInfo0.fragShaderModule,VULKAN_CPU_ALLOCATOR);
	vkDestroyShaderModule(m_Device,pipelineInfo1.vertShaderModule,VULKAN_CPU_ALLOCATOR);
	vkDestroyShaderModule(m_Device,pipelineInfo1.fragShaderModule,VULKAN_CPU_ALLOCATOR);
	vkDestroyShaderModule(m_Device,pipelineInfo2.vertShaderModule,VULKAN_CPU_ALLOCATOR);
	vkDestroyShaderModule(m_Device,pipelineInfo2.fragShaderModule,VULKAN_CPU_ALLOCATOR);
}

void PipeLines::DestroyPipelines()
{
	for(int32 i = 0; i<m_Pipelines.size(); ++i)
	{
		delete m_Pipelines[i];
	}
	m_Pipelines.clear();
}

void PipeLines::DestroyUniformBuffers()
{
	m_MVPBuffer->UnMap();
	KK_DELETE(m_MVPBuffer,DVKBuffer);
	m_ParamBuffer->UnMap();
	KK_DELETE(m_ParamBuffer,DVKBuffer);
}
