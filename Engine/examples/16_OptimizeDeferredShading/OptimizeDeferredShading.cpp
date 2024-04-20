#include "OptimizeDeferredShading.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<OptimizeDeferredShading>(1400,900,"OptimizeDeferredShading",cmdLine);
}

OptimizeDeferredShading::OptimizeDeferredShading(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

OptimizeDeferredShading::~OptimizeDeferredShading()
{
}

bool OptimizeDeferredShading::PreInit()
{
	return true;
}

bool OptimizeDeferredShading::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	LoadAssets();
	CreateGUI();
	CreateUniformBuffers();
	CreateDescriptorSet();
	CreatePipelines();
	SetupCommandBuffers();

	m_Ready = true;

	return true;
}

void OptimizeDeferredShading::Exist()
{
	ModuleBase::Release();

	DestroyAssets();
	DestroyGUI();
	DestroyPipelines();
	DestroyUniformBuffers();

	DestroyAttachments();
}

void OptimizeDeferredShading::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}


void OptimizeDeferredShading::CreateFrameBuffers()
{
	DestroyFrameBuffers();

	int32 fwidth = GetVulkanRHI()->GetSwapChain()->GetWidth();
	int32 fheight = GetVulkanRHI()->GetSwapChain()->GetHeight();
	VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

	VkImageView attachments[4];

	VkFramebufferCreateInfo frameBufferCreateInfo;
	ZeroVulkanStruct(frameBufferCreateInfo,VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
	frameBufferCreateInfo.renderPass = m_RenderPass;
	frameBufferCreateInfo.attachmentCount = 4;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = fwidth;
	frameBufferCreateInfo.height = fheight;
	frameBufferCreateInfo.layers = 1;

	const std::vector<VkImageView>& backbufferViews = GetVulkanRHI()->GetBackbufferViews();

	m_FrameBuffers.resize(backbufferViews.size());
	for(uint32 i = 0; i<m_FrameBuffers.size(); ++i)
	{
		attachments[0] = backbufferViews[i];
		attachments[1] = m_AttachsColor[i]->imageView;
		attachments[2] = m_AttachsNormal[i]->imageView;
		attachments[3] = m_AttachsDepth[i]->imageView;
		VERIFYVULKANRESULT(vkCreateFramebuffer(device,&frameBufferCreateInfo,VULKAN_CPU_ALLOCATOR,&m_FrameBuffers[i]));
	}
}

void OptimizeDeferredShading::CreateDepthStencil()
{

}

void OptimizeDeferredShading::DestoryDepthStencil()
{

}

void OptimizeDeferredShading::DestroyAttachments()
{
	for(int32 i = 0; i<m_AttachsDepth.size(); ++i)
	{
		DVKTexture* texture = m_AttachsDepth[i];
		delete texture;
	}
	m_AttachsDepth.clear();

	for(int32 i = 0; i<m_AttachsColor.size(); ++i)
	{
		DVKTexture* texture = m_AttachsColor[i];
		delete texture;
	}
	m_AttachsColor.clear();

	for(int32 i = 0; i<m_AttachsNormal.size(); ++i)
	{
		DVKTexture* texture = m_AttachsNormal[i];
		delete texture;
	}
	m_AttachsNormal.clear();
}

void OptimizeDeferredShading::CreateAttachments()
{
	auto swapChain = GetVulkanRHI()->GetSwapChain();
	int32 fwidth = swapChain->GetWidth();
	int32 fheight = swapChain->GetHeight();
	int32 numBuffer = swapChain->GetBackBufferCount();

	m_AttachsColor.resize(numBuffer);
	m_AttachsNormal.resize(numBuffer);
	m_AttachsDepth.resize(numBuffer);

	for(int32 i = 0; i<m_AttachsColor.size(); ++i)
	{
		m_AttachsColor[i] = DVKTexture::CreateAttachment(
			m_VulkanDevice,
			PixelFormatToVkFormat(GetVulkanRHI()->GetPixelFormat(),false),
			VK_IMAGE_ASPECT_COLOR_BIT,
			fwidth,
			fheight,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
		);
	}

	for(int32 i = 0; i<m_AttachsNormal.size(); ++i)
	{
		m_AttachsNormal[i] = DVKTexture::CreateAttachment(
			m_VulkanDevice,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			fwidth,
			fheight,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
		);
	}

	for(int32 i = 0; i<m_AttachsDepth.size(); ++i)
	{
		m_AttachsDepth[i] = DVKTexture::CreateAttachment(
			m_VulkanDevice,
			PixelFormatToVkFormat(m_DepthFormat,false),
			VK_IMAGE_ASPECT_DEPTH_BIT,
			fwidth,
			fheight,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT
		);
	}
}

void OptimizeDeferredShading::CreateRenderPass()
{
	DestoryRenderPass();
	DestroyAttachments();
	CreateAttachments();

	VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
	PixelFormat pixelFormat = GetVulkanRHI()->GetPixelFormat();

	std::vector<VkAttachmentDescription> attachments(4);
	// swap chain attachment
	attachments[0].format = PixelFormatToVkFormat(pixelFormat,false);
	attachments[0].samples = m_SampleCount;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// color attachment
	attachments[1].format = PixelFormatToVkFormat(pixelFormat,false);
	attachments[1].samples = m_SampleCount;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// normal attachment
	attachments[2].format = VK_FORMAT_R8G8B8A8_UNORM;
	attachments[2].samples = m_SampleCount;
	attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// depth stencil attachment
	attachments[3].format = PixelFormatToVkFormat(m_DepthFormat,false);
	attachments[3].samples = m_SampleCount;
	attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReferences[2];
	colorReferences[0].attachment = 1;
	colorReferences[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	colorReferences[1].attachment = 2;
	colorReferences[1].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference swapReference = { };
	swapReference.attachment = 0;
	swapReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = { };
	depthReference.attachment = 3;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference inputReferences[3];
	inputReferences[0].attachment = 1;
	inputReferences[0].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[1].attachment = 2;
	inputReferences[1].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	inputReferences[2].attachment = 3;
	inputReferences[2].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	std::vector<VkSubpassDescription> subpassDescriptions(2);
	subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[0].colorAttachmentCount = 2;
	subpassDescriptions[0].pColorAttachments = colorReferences;
	subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

	subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescriptions[1].colorAttachmentCount = 1;
	subpassDescriptions[1].pColorAttachments = &swapReference;
	subpassDescriptions[1].inputAttachmentCount = 3;
	subpassDescriptions[1].pInputAttachments = inputReferences;

	std::vector<VkSubpassDependency> dependencies(3);
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = 1;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[2].srcSubpass = 1;
	dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo;
	ZeroVulkanStruct(renderPassInfo,VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = subpassDescriptions.size();
	renderPassInfo.pSubpasses = subpassDescriptions.data();
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();
	VERIFYVULKANRESULT(vkCreateRenderPass(device,&renderPassInfo,VULKAN_CPU_ALLOCATOR,&m_RenderPass));
}

void OptimizeDeferredShading::DestroyFrameBuffers()
{
	VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
	for(int32 i = 0; i<m_FrameBuffers.size(); ++i)
	{
		vkDestroyFramebuffer(device,m_FrameBuffers[i],VULKAN_CPU_ALLOCATOR);
	}
	m_FrameBuffers.clear();
}

void OptimizeDeferredShading::DestoryRenderPass()
{
	VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
	if(m_RenderPass!=VK_NULL_HANDLE)
	{
		vkDestroyRenderPass(device,m_RenderPass,VULKAN_CPU_ALLOCATOR);
		m_RenderPass = VK_NULL_HANDLE;
	}
}


void OptimizeDeferredShading::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	UpdateUniform(time,delta);

	ModuleBase::Present(bufferIndex);
}

void OptimizeDeferredShading::UpdateUniform(float time,float delta)
{
	m_ViewCamera.Perspective(PI/2,GetWidth(),GetHeight(),m_VertFragParam.zNear,m_VertFragParam.zFar);
	m_ViewProjData.view = m_ViewCamera.GetView();
	m_ViewProjData.projection = m_ViewCamera.GetProjection();
	m_ViewProjBuffer->CopyFrom(&m_ViewProjData,sizeof(ViewProjectionBlock));

	m_VertFragParam.invView = m_ViewCamera.GetView();
	m_VertFragParam.invView.SetInverse();
	m_VertFragParam.yMaxFar = m_VertFragParam.zFar*MMath::Tan(MMath::DegreesToRadians(45.0f)/2);
	m_VertFragParam.xMaxFar = m_VertFragParam.yMaxFar*(float)GetWidth()/(float)GetHeight();
	m_ParamBuffer->CopyFrom(&m_VertFragParam,sizeof(AttachmentParamBlock));

	for(int32 i = 0; i<NUM_LIGHTS; ++i)
	{
		float bias = MMath::Sin(time*m_LightInfos.speed[i])/5.0f;
		m_LightDatas.lights[i].position.x = m_LightInfos.position[i].x+bias*m_LightInfos.direction[i].x*500.0f;
		m_LightDatas.lights[i].position.y = m_LightInfos.position[i].y+bias*m_LightInfos.direction[i].y*500.0f;
		m_LightDatas.lights[i].position.z = m_LightInfos.position[i].z+bias*m_LightInfos.direction[i].z*500.0f;
	}
	m_LightParamBuffer->CopyFrom(&m_LightDatas,sizeof(LightDataBlock));
}

bool OptimizeDeferredShading::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("OptimizeDeferredShading",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		int32 index = m_VertFragParam.attachmentIndex;
		ImGui::SliderInt("Index",&index,0,3);
		m_VertFragParam.attachmentIndex = index;

		if(ImGui::Button("Random"))
		{
			DVKBoundingBox bounds = m_Model->rootNode->GetBounds();

			for(int32 i = 0; i<NUM_LIGHTS; ++i)
			{
				m_LightDatas.lights[i].position.x = MMath::RandRange(bounds.min.x,bounds.max.x);
				m_LightDatas.lights[i].position.y = MMath::RandRange(bounds.min.y,bounds.max.y);
				m_LightDatas.lights[i].position.z = MMath::RandRange(bounds.min.z,bounds.max.z);

				m_LightDatas.lights[i].color.x = MMath::RandRange(0.0f,1.0f);
				m_LightDatas.lights[i].color.y = MMath::RandRange(0.0f,1.0f);
				m_LightDatas.lights[i].color.z = MMath::RandRange(0.0f,1.0f);

				m_LightDatas.lights[i].radius = MMath::RandRange(50.0f,200.0f);

				m_LightInfos.position[i] = m_LightDatas.lights[i].position;
				m_LightInfos.direction[i] = m_LightInfos.position[i];
				m_LightInfos.direction[i].Normalize();
				m_LightInfos.speed[i] = 1.0f+MMath::RandRange(0.0f,5.0f);
			}
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

void OptimizeDeferredShading::LoadAssets()
{
	m_Shader0 = DVKShader::Create(
		m_VulkanDevice,
		"Assets/Shaders/16_OptimizeDeferredShading/obj.vert.spv",
		"Assets/Shaders/16_OptimizeDeferredShading/obj.frag.spv"
	);

	m_Shader1 = DVKShader::Create(
		m_VulkanDevice,
		"Assets/Shaders/16_OptimizeDeferredShading/quad.vert.spv",
		"Assets/Shaders/16_OptimizeDeferredShading/quad.frag.spv"
	);

	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// scene model
	m_Model = DVKModel::LoadFromFile(
		"Assets/models/Room/miniHouse_FBX.FBX",
		m_VulkanDevice,
		cmdBuffer,
		m_Shader0->perVertexAttributes
	);

	// quad model
	std::vector<float> vertices = {
		-1.0f,1.0f,0.0f,0.0f,0.0f,
		1.0f,1.0f,0.0f,1.0f,0.0f,
		1.0f,-1.0f,0.0f,1.0f,1.0f,
		-1.0f,-1.0f,0.0f,0.0f,1.0f
	};
	std::vector<uint16> indices = {
		0,1,2,0,2,3
	};

	m_Quad = DVKModel::Create(
		m_VulkanDevice,
		cmdBuffer,
		vertices,
		indices,
		m_Shader1->perVertexAttributes
	);

	delete cmdBuffer;
}

void OptimizeDeferredShading::DestroyAssets()
{
	delete m_Model;
	delete m_Quad;

	delete m_Shader0;
	delete m_Shader1;
}

void OptimizeDeferredShading::SetupCommandBuffers()
{
	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);

	VkClearValue clearValues[4];
	clearValues[0].color = {
		{ 0.2f,0.2f,0.2f,0.0f }
	};
	clearValues[1].color = {
		{ 0.0f,0.0f,0.0f,1.0f }
	};
	clearValues[2].color = {
		{ 0.2f,0.2f,0.2f,0.0f }
	};
	clearValues[3].depthStencil = { 1.0f,0 };

	VkRenderPassBeginInfo renderPassBeginInfo;
	ZeroVulkanStruct(renderPassBeginInfo,VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.clearValueCount = 4;
	renderPassBeginInfo.pClearValues = clearValues;
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_FrameWidth;
	renderPassBeginInfo.renderArea.extent.height = m_FrameHeight;

	VkViewport viewport = { };
	viewport.x = 0;
	viewport.y = m_FrameHeight;
	viewport.width = m_FrameWidth;
	viewport.height = -(float)m_FrameHeight;    // flip y axis
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.extent.width = m_FrameWidth;
	scissor.extent.height = m_FrameHeight;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	uint32 alignment = m_VulkanDevice->GetLimits().minUniformBufferOffsetAlignment;
	uint32 modelAlign = Align(sizeof(ModelBlock),alignment);

	for(int32 i = 0; i<m_CommandBuffers.size(); ++i)
	{
		renderPassBeginInfo.framebuffer = m_FrameBuffers[i];

		VERIFYVULKANRESULT(vkBeginCommandBuffer(m_CommandBuffers[i],&cmdBeginInfo));
		vkCmdBeginRenderPass(m_CommandBuffers[i],&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

		vkCmdSetViewport(m_CommandBuffers[i],0,1,&viewport);
		vkCmdSetScissor(m_CommandBuffers[i],0,1,&scissor);

		// pass0
		{
			vkCmdBindPipeline(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline0->pipeline);
			for(int32 meshIndex = 0; meshIndex<m_Model->meshes.size(); ++meshIndex)
			{
				uint32 offset = meshIndex*modelAlign;
				vkCmdBindDescriptorSets(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline0->pipelineLayout,0,m_DescriptorSet0->descriptorSets.size(),m_DescriptorSet0->descriptorSets.data(),1,&offset);
				m_Model->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);
			}
		}

		vkCmdNextSubpass(m_CommandBuffers[i],VK_SUBPASS_CONTENTS_INLINE);

		// pass1
		{
			vkCmdBindPipeline(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline1->pipeline);
			vkCmdBindDescriptorSets(m_CommandBuffers[i],VK_PIPELINE_BIND_POINT_GRAPHICS,m_Pipeline1->pipelineLayout,0,m_DescriptorSets[i]->descriptorSets.size(),m_DescriptorSets[i]->descriptorSets.data(),0,nullptr);
			for(int32 meshIndex = 0; meshIndex<m_Quad->meshes.size(); ++meshIndex)
			{
				m_Quad->meshes[meshIndex]->BindDrawCmd(m_CommandBuffers[i]);
			}
		}

		m_GUI->BindDrawCmd(m_CommandBuffers[i],m_RenderPass,1);

		vkCmdEndRenderPass(m_CommandBuffers[i]);
		VERIFYVULKANRESULT(vkEndCommandBuffer(m_CommandBuffers[i]));
	}
}

void OptimizeDeferredShading::CreateDescriptorSet()
{
	m_ModelBufferInfo.buffer = m_ModelBuffer->buffer;
	m_ModelBufferInfo.offset = 0;
	m_ModelBufferInfo.range = sizeof(ModelBlock);

	m_DescriptorSet0 = m_Shader0->AllocateDescriptorSet();
	m_DescriptorSet0->WriteBuffer("uboViewProj",m_ViewProjBuffer);
	m_DescriptorSet0->WriteBuffer("uboModel",&m_ModelBufferInfo);

	m_DescriptorSets.resize(m_AttachsColor.size());
	for(int32 i = 0; i<m_DescriptorSets.size(); ++i)
	{
		m_DescriptorSets[i] = m_Shader1->AllocateDescriptorSet();
		m_DescriptorSets[i]->WriteImage("inputColor",m_AttachsColor[i]);
		m_DescriptorSets[i]->WriteImage("inputNormal",m_AttachsNormal[i]);
		m_DescriptorSets[i]->WriteImage("inputDepth",m_AttachsDepth[i]);
		m_DescriptorSets[i]->WriteBuffer("paramData",m_ParamBuffer);
		m_DescriptorSets[i]->WriteBuffer("lightDatas",m_LightParamBuffer);
	}
}

void OptimizeDeferredShading::CreatePipelines()
{
	DVKGfxPipelineInfo pipelineInfo0;
	pipelineInfo0.shader = m_Shader0;
	pipelineInfo0.colorAttachmentCount = 2;
	m_Pipeline0 = DVKGfxPipeline::Create(
		m_VulkanDevice,
		m_PipelineCache,
		pipelineInfo0,
		{
			m_Model->GetInputBinding()
		},
		m_Model->GetInputAttributes(),
		m_Shader0->pipelineLayout,
		m_RenderPass
	);

	DVKGfxPipelineInfo pipelineInfo1;
	pipelineInfo1.depthStencilState.depthTestEnable = VK_FALSE;
	pipelineInfo1.depthStencilState.depthWriteEnable = VK_FALSE;
	pipelineInfo1.depthStencilState.stencilTestEnable = VK_FALSE;
	pipelineInfo1.shader = m_Shader1;
	pipelineInfo1.subpass = 1;
	m_Pipeline1 = DVKGfxPipeline::Create(
		m_VulkanDevice,
		m_PipelineCache,
		pipelineInfo1,
		{
			m_Quad->GetInputBinding()
		},
		m_Quad->GetInputAttributes(),
		m_Shader1->pipelineLayout,
		m_RenderPass
	);
}

void OptimizeDeferredShading::DestroyPipelines()
{
	delete m_Pipeline0;
	delete m_Pipeline1;

	delete m_DescriptorSet0;
	for(int32 i = 0; i<m_DescriptorSets.size(); ++i)
	{
		DVKDescriptorSet* descriptorSet = m_DescriptorSets[i];
		delete descriptorSet;
	}
	m_DescriptorSets.clear();
}

void OptimizeDeferredShading::CreateUniformBuffers()
{
	DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;

	// param
	m_VertFragParam.attachmentIndex = 0;
	m_VertFragParam.zNear = 10.0f;
	m_VertFragParam.zFar = 3000.0f;
	m_VertFragParam.one = 1.0f;
	m_VertFragParam.yMaxFar = m_VertFragParam.zFar*MMath::Tan(MMath::DegreesToRadians(45.0f)/2);
	m_VertFragParam.xMaxFar = m_VertFragParam.yMaxFar*(float)GetWidth()/(float)GetHeight();

	m_ViewCamera.Perspective(PI/4,GetWidth(),GetHeight(),m_VertFragParam.zNear,m_VertFragParam.zFar);
	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y+1000,boundCenter.z-boundSize.Size());
	m_ViewCamera.LookAt(boundCenter);

	m_VertFragParam.invView = m_ViewCamera.GetView();
	m_VertFragParam.invView.SetInverse();

	m_ViewProjBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(ViewProjectionBlock),
		&(m_ViewProjData)
	);
	m_ViewProjBuffer->Map();

	// dynamic
	uint32 alignment = m_VulkanDevice->GetLimits().minUniformBufferOffsetAlignment;
	uint32 modelAlign = Align(sizeof(ModelBlock),alignment);
	m_ModelDatas.resize(modelAlign*m_Model->meshes.size());
	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		ModelBlock* modelBlock = (ModelBlock*)(m_ModelDatas.data()+modelAlign*i);
		modelBlock->model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
	}

	m_ModelBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		m_ModelDatas.size(),
		m_ModelDatas.data()
	);
	m_ModelBuffer->Map();

	// param buffer
	m_ParamBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(AttachmentParamBlock),
		&(m_VertFragParam)
	);
	m_ParamBuffer->Map();

	// light datas
	for(int32 i = 0; i<NUM_LIGHTS; ++i)
	{
		m_LightDatas.lights[i].position.x = MMath::RandRange(bounds.min.x,bounds.max.x);
		m_LightDatas.lights[i].position.y = MMath::RandRange(bounds.min.y,bounds.max.y);
		m_LightDatas.lights[i].position.z = MMath::RandRange(bounds.min.z,bounds.max.z);
		m_LightDatas.lights[i].position.w = 1.0f;

		m_LightDatas.lights[i].color.x = MMath::RandRange(0.0f,1.0f);
		m_LightDatas.lights[i].color.y = MMath::RandRange(0.0f,1.0f);
		m_LightDatas.lights[i].color.z = MMath::RandRange(0.0f,1.0f);

		m_LightDatas.lights[i].radius = MMath::RandRange(50.0f,200.0f);

		m_LightInfos.position[i] = m_LightDatas.lights[i].position;
		m_LightInfos.direction[i] = m_LightInfos.position[i];
		m_LightInfos.direction[i].Normalize();
		m_LightInfos.speed[i] = 1.0f+MMath::RandRange(0.0f,5.0f);
	}
	m_LightParamBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		sizeof(LightDataBlock),
		&(m_LightDatas)
	);
	m_LightParamBuffer->Map();

	{
		// fast reconstruct position from eye linear depth
		Matrix4x4 modelViewProj;
		modelViewProj.Append(m_ViewProjData.view);
		modelViewProj.Append(m_ViewProjData.projection);

		// real world position
		Vector4 realPos(10,20,30,1.0f);
		// view space position
		Vector4 posView = m_ViewProjData.view.TransformPosition(realPos);
		// projection space position
		Vector4 posProj = modelViewProj.TransformVector4(realPos);
		// none linear depth
		float depth = posProj.z/posProj.w;
		// linear depth
		float zc0 = 1.0-m_VertFragParam.zFar/m_VertFragParam.zNear;
		float zc1 = m_VertFragParam.zFar/m_VertFragParam.zNear;
		float depth01 = 1.0/(zc0*depth+zc1);
		MLOG("LinearDepth:%f,%f",posProj.w/m_VertFragParam.zFar,depth01);
		// ndc space
		float u = posProj.x/posProj.w;
		float v = posProj.y/posProj.w;
		// view space ray
		Vector3 viewRay = Vector3(m_VertFragParam.xMaxFar*u,m_VertFragParam.yMaxFar*v,m_VertFragParam.zFar);
		Vector3 viewPos = viewRay*depth01;
		MLOG("posView:(%f,%f,%f) - (%f,%f,%f)",posView.x,posView.y,posView.z,viewPos.x,viewPos.y,viewPos.z);
	}

}

void OptimizeDeferredShading::DestroyUniformBuffers()
{
	m_ViewProjBuffer->UnMap();
	delete m_ViewProjBuffer;
	m_ViewProjBuffer = nullptr;

	m_ModelBuffer->UnMap();
	delete m_ModelBuffer;
	m_ModelBuffer = nullptr;

	m_ParamBuffer->UnMap();
	delete m_ParamBuffer;
	m_ParamBuffer = nullptr;

	m_LightParamBuffer->UnMap();
	delete m_LightParamBuffer;
	m_LightParamBuffer = nullptr;
}

void OptimizeDeferredShading::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void OptimizeDeferredShading::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}