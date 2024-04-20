#include "MSAADemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<MSAADemo>(1400,900,"MSAADemo",cmdLine);
}

MSAADemo::MSAADemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

MSAADemo::~MSAADemo()
{
}

 bool MSAADemo::Init() 
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	LoadAssets();
	InitParmas();
	CreateGUI();

	m_Ready = true;

	return true;
}

 void MSAADemo::Exist() 
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
}

 void MSAADemo::Loop(float time,float delta) 
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void MSAADemo::CreateMSAATexture()
{
	// msaa color texture
	m_MSAAColorTexture = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(GetVulkanRHI()->GetPixelFormat(),false),
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT|VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
		m_MSAACount
	);

	// msaa depth texture
	m_MSAADepthTexture = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(m_DepthFormat,false),
		VK_IMAGE_ASPECT_DEPTH_BIT|VK_IMAGE_ASPECT_STENCIL_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT|VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		m_MSAACount
	);
}

void MSAADemo::DestroyMSAATexture()
{
	if(m_MSAAColorTexture)
	{
		delete m_MSAAColorTexture;
		m_MSAAColorTexture = nullptr;
	}

	if(m_MSAADepthTexture)
	{
		delete m_MSAADepthTexture;
		m_MSAADepthTexture = nullptr;
	}
}

void MSAADemo::CreateMSAAFrameBuffers()
{
	DestroyMSAATexture();
	CreateMSAATexture();

	int32 fwidth = GetVulkanRHI()->GetSwapChain()->GetWidth();
	int32 fheight = GetVulkanRHI()->GetSwapChain()->GetHeight();
	VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

	std::vector<VkImageView> attachments(4);
	attachments[0] = m_MSAAColorTexture->imageView;
	// attachments[1] = swapchain image
	attachments[2] = m_MSAADepthTexture->imageView;
	attachments[3] = m_DepthStencilView;

	VkFramebufferCreateInfo frameBufferCreateInfo;
	ZeroVulkanStruct(frameBufferCreateInfo,VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
	frameBufferCreateInfo.renderPass = m_RenderPass;
	frameBufferCreateInfo.attachmentCount = attachments.size();
	frameBufferCreateInfo.pAttachments = attachments.data();
	frameBufferCreateInfo.width = fwidth;
	frameBufferCreateInfo.height = fheight;
	frameBufferCreateInfo.layers = 1;

	const std::vector<VkImageView>& backbufferViews = GetVulkanRHI()->GetBackbufferViews();

	m_FrameBuffers.resize(backbufferViews.size());
	for(uint32 i = 0; i<m_FrameBuffers.size(); ++i)
	{
		attachments[1] = backbufferViews[i];
		VERIFYVULKANRESULT(vkCreateFramebuffer(device,&frameBufferCreateInfo,VULKAN_CPU_ALLOCATOR,&m_FrameBuffers[i]));
	}
}

void MSAADemo::CreateFrameBuffers() 
{
	DestroyFrameBuffers();
	DestroyMSAATexture();

	if(m_MSAAEnable)
	{
		CreateMSAAFrameBuffers();
	}
	else
	{
		ModuleBase::CreateFrameBuffers();
	}
}

void MSAADemo::CreateMSAARenderPass()
{
	VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
	PixelFormat pixelFormat = GetVulkanRHI()->GetPixelFormat();

	std::vector<VkAttachmentDescription> attachments(4);
	// MSAA Attachment
	attachments[0].format = PixelFormatToVkFormat(pixelFormat,false);
	attachments[0].samples = m_MSAACount;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	// color attachment
	attachments[1].format = PixelFormatToVkFormat(pixelFormat,false);
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	// MSAA Depth
	attachments[2].format = PixelFormatToVkFormat(m_DepthFormat,false);
	attachments[2].samples = m_MSAACount;
	attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	// depth stencil attachment
	attachments[3].format = PixelFormatToVkFormat(m_DepthFormat,false);
	attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[3].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference;
	colorReference.attachment = 0;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference;
	depthReference.attachment = 2;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference resolveReference;
	resolveReference.attachment = 1;
	resolveReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = { };
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorReference;
	subpass.pResolveAttachments = &resolveReference;
	subpass.pDepthStencilAttachment = &depthReference;

	std::vector<VkSubpassDependency> dependencies(2);
	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT|VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo;
	ZeroVulkanStruct(renderPassInfo,VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = dependencies.size();
	renderPassInfo.pDependencies = dependencies.data();
	VERIFYVULKANRESULT(vkCreateRenderPass(device,&renderPassInfo,VULKAN_CPU_ALLOCATOR,&m_RenderPass));
}

void MSAADemo::CreateRenderPass() 
{
	DestoryRenderPass();
	if(m_MSAAEnable)
	{
		CreateMSAARenderPass();
	}
	else
	{
		ModuleBase::CreateRenderPass();
	}
}

void MSAADemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	m_MVPData.view = m_ViewCamera.GetView();
	m_MVPData.projection = m_ViewCamera.GetProjection();

	m_LineModel->rootNode->localMatrix.AppendRotation(delta*15.0f,Vector3::UpVector);

	DVKMaterial* material = m_MSAAEnable ? m_MSAAMaterial : m_NoneMaterial;
	material->BeginFrame();
	for(int32 i = 0; i<m_LineModel->meshes.size(); ++i)
	{
		DVKMesh* mesh = m_LineModel->meshes[i];
		m_MVPData.model = mesh->linkNode->GetGlobalMatrix();
		material->BeginObject();
		material->SetLocalUniform("uboMVP",&m_MVPData,sizeof(ModelViewProjectionBlock));
		material->EndObject();
	}
	material->EndFrame();

	SetupCommandBuffers(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool MSAADemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("MSAADemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		if(ImGui::Checkbox("MSAA",&m_MSAAEnable))
		{
			CreateRenderPass();
			CreateFrameBuffers();
			CreateMaterials();
		}

		if(m_MSAAEnable)
		{
			ImGui::Text("MSAA:%d",int32(m_MSAACount));
		}

		ImGui::Text("%.3f ms/frame (%.1f FPS)",1000.0f/ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void MSAADemo::GenerateLineSphere(std::vector<float>& outVertices,int32 sphslices,float scale)
{
	int32 count = 0;
	int32 slices = sphslices;
	int32 stacks = slices;

	outVertices.resize((slices+1)*stacks*3*2);

	float ds = 1.0f/sphslices;
	float dt = 1.0f/sphslices;
	float t = 1.0;
	float drho = PI/stacks;
	float dtheta = 2.0*PI/slices;

	for(int32 i = 0; i<stacks; ++i)
	{
		float rho = i*drho;
		float s = 0.0;
		for(int32 j = 0; j<=slices; ++j)
		{
			float theta = (j==slices) ? 0.0f : j*dtheta;
			float x = -sin(theta)*sin(rho)*scale;
			float z = cos(theta)*sin(rho)*scale;
			float y = -cos(rho)*scale;

			outVertices[count+0] = x;
			outVertices[count+1] = y;
			outVertices[count+2] = z;
			count += 3;

			x = -sin(theta)*sin(rho+drho)*scale;
			z = cos(theta)*sin(rho+drho)*scale;
			y = -cos(rho+drho)*scale;

			outVertices[count+0] = x;
			outVertices[count+1] = y;
			outVertices[count+2] = z;
			count += 3;

			s += ds;
		}
		t -= dt;
	}
}

VkSampleCountFlagBits MSAADemo::GetMaxUsableSampleCount()
{
	VkSampleCountFlags counts = MMath::Min(m_VulkanDevice->GetLimits().framebufferColorSampleCounts,m_VulkanDevice->GetLimits().framebufferDepthSampleCounts);
	if(counts&VK_SAMPLE_COUNT_64_BIT)
	{
		return VK_SAMPLE_COUNT_64_BIT;
	}
	if(counts&VK_SAMPLE_COUNT_32_BIT)
	{
		return VK_SAMPLE_COUNT_32_BIT;
	}
	if(counts&VK_SAMPLE_COUNT_16_BIT)
	{
		return VK_SAMPLE_COUNT_16_BIT;
	}
	if(counts&VK_SAMPLE_COUNT_8_BIT)
	{
		return VK_SAMPLE_COUNT_8_BIT;
	}
	if(counts&VK_SAMPLE_COUNT_4_BIT)
	{
		return VK_SAMPLE_COUNT_4_BIT;
	}
	if(counts&VK_SAMPLE_COUNT_2_BIT)
	{
		return VK_SAMPLE_COUNT_2_BIT;
	}
	return VK_SAMPLE_COUNT_1_BIT;
}

void MSAADemo::DestroyMaterials()
{
	if(m_MSAAMaterial)
	{
		delete m_MSAAMaterial;
		m_MSAAMaterial = nullptr;
	}

	if(m_NoneMaterial)
	{
		delete m_NoneMaterial;
		m_NoneMaterial = nullptr;
	}
}

void MSAADemo::CreateMaterials()
{
	DestroyMaterials();

	float range0 = m_VulkanDevice->GetLimits().lineWidthRange[0];
	float range1 = m_VulkanDevice->GetLimits().lineWidthRange[1];
	float lineWidth = MMath::Clamp(3.0f,range0,range1);

	// msaa material
	if(m_MSAAEnable)
	{
		m_MSAAMaterial = DVKMaterial::Create(
			m_VulkanDevice,
			m_RenderPass,
			m_PipelineCache,
			m_LineShader
		);
		m_MSAAMaterial->pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		m_MSAAMaterial->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
		m_MSAAMaterial->pipelineInfo.rasterizationState.lineWidth = lineWidth;
		m_MSAAMaterial->pipelineInfo.rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		m_MSAAMaterial->pipelineInfo.multisampleState.rasterizationSamples = m_MSAACount;
		m_MSAAMaterial->PreparePipeline();
	}
	else
	{
		// none msaa material
		m_NoneMaterial = DVKMaterial::Create(
			m_VulkanDevice,
			m_RenderPass,
			m_PipelineCache,
			m_LineShader
		);
		m_NoneMaterial->pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
		m_NoneMaterial->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
		m_NoneMaterial->pipelineInfo.rasterizationState.lineWidth = lineWidth;
		m_NoneMaterial->pipelineInfo.rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
		m_NoneMaterial->PreparePipeline();
	}
}

void MSAADemo::LoadAssets()
{
	m_MSAACount = GetMaxUsableSampleCount();

	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// LineSphere
	std::vector<float> vertices;
	GenerateLineSphere(vertices,40,1.0f);

	// model
	m_LineModel = DVKModel::Create(
		m_VulkanDevice,
		cmdBuffer,
		vertices,
		{ },
		{ VertexAttribute::VA_Position }
	);

	// shader
	m_LineShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/31_MSAA/obj.vert.spv",
		"Assets/Shaders/31_MSAA/obj.frag.spv"
	);

	CreateMaterials();

	delete cmdBuffer;
}

void MSAADemo::DestroyAssets()
{
	DestroyMaterials();

	delete m_LineShader;
	delete m_LineModel;

	DestroyMSAATexture();
}

void MSAADemo::SetupCommandBuffers(int32 backBufferIndex)
{
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

	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	std::vector<VkClearValue> clearValues;
	if(m_MSAAEnable)
	{
		clearValues.resize(3);
		clearValues[0].color = {
			{ 0.2f,0.2f,0.2f,1.0f }
		};
		clearValues[1].color = {
			{ 0.2f,0.2f,0.2f,1.0f }
		};
		clearValues[2].depthStencil = { 1.0f,0 };
	}
	else
	{
		clearValues.resize(2);
		clearValues[0].color = {
			{ 0.2f,0.2f,0.2f,1.0f }
		};
		clearValues[1].depthStencil = { 1.0f,0 };
	}

	VkRenderPassBeginInfo renderPassBeginInfo;
	ZeroVulkanStruct(renderPassBeginInfo,VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
	renderPassBeginInfo.renderPass = m_RenderPass;
	renderPassBeginInfo.framebuffer = m_FrameBuffers[backBufferIndex];
	renderPassBeginInfo.clearValueCount = clearValues.size();
	renderPassBeginInfo.pClearValues = clearValues.data();
	renderPassBeginInfo.renderArea.offset.x = 0;
	renderPassBeginInfo.renderArea.offset.y = 0;
	renderPassBeginInfo.renderArea.extent.width = m_FrameWidth;
	renderPassBeginInfo.renderArea.extent.height = m_FrameHeight;
	vkCmdBeginRenderPass(commandBuffer,&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(commandBuffer,0,1,&viewport);
	vkCmdSetScissor(commandBuffer,0,1,&scissor);

	DVKMaterial* material = m_MSAAEnable ? m_MSAAMaterial : m_NoneMaterial;
	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,material->GetPipeline());
	for(int32 j = 0; j<m_LineModel->meshes.size(); ++j)
	{
		material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,j);
		m_LineModel->meshes[j]->BindDrawCmd(commandBuffer);
	}

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass,0,m_MSAAEnable ? m_MSAACount : VK_SAMPLE_COUNT_1_BIT);

	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void MSAADemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,0.0f,-3.0f);
	m_ViewCamera.Perspective(PI/4,GetWidth(),GetHeight(),0.1f,1000.0f);
}

void MSAADemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void MSAADemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}