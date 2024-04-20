#include "ThreadedRenderingDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<ThreadedRenderingDemo>(1400,900,"ThreadedRenderingDemo",cmdLine);
}

ThreadedRenderingDemo::ThreadedRenderingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

ThreadedRenderingDemo::~ThreadedRenderingDemo()
{

}
bool ThreadedRenderingDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	LoadAnimModel();
	LoadAssets();
	InitParmas();
	InitThreads();

	m_Ready = true;
	return true;
}

void ThreadedRenderingDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void ThreadedRenderingDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}


void ThreadedRenderingDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	m_bufferIndex = bufferIndex;
	m_FrameTime = time;
	m_FrameDelta = delta;

	UpdateFPS(time,delta);

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	UpdateAnimation(time,delta);

	// notify fram start
	{
		std::lock_guard<std::mutex> lockGuard(m_FrameStartLock);
		m_ThreadDoneCount = 0;
		m_MainFrameID += 1;
		m_FrameStartCV.notify_all();
	}

	// wait for thread done
	{
		std::unique_lock<std::mutex> lockGuard(m_ThreadDoneLock);
		while(m_ThreadDoneCount!=m_Threads.size())
		{
			m_ThreadDoneCV.wait(lockGuard);
		}
	}

	SetupCommandBuffers(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool ThreadedRenderingDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("ThreadedRenderingDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void ThreadedRenderingDemo::UpdateAnimation(float time,float delta)
{
	m_RoleModel->Update(time,delta);

	DVKMesh* mesh = m_RoleModel->meshes[0];
	for(int32 i = 0; i<mesh->bones.size(); ++i)
	{
		int32 index = mesh->bones[i];
		m_BonesData[index] = m_RoleModel->bones[index]->finalTransform;
	}
}

void ThreadedRenderingDemo::LoadAnimModel()
{
	m_RoleModel = DVKModel::LoadFromFile(
		"Assets/models/xiaonan/nvhai.fbx",
		m_VulkanDevice,
		nullptr,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_Normal,
			VertexAttribute::VA_SkinIndex,
			VertexAttribute::VA_SkinWeight
		}
	);

	m_RoleModel->SetAnimation(0);
	m_BonesData.resize(m_RoleModel->meshes[0]->bones.size());
}

void ThreadedRenderingDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// fullscreen
	m_Quad = DVKDefaultRes::fullQuad;

	// scene model
	m_ParticleModel = DVKModel::LoadFromFile(
		"Assets/models/plane_z.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0,
		}
	);

	m_ParticleShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/54_ThreadedRendering/obj.vert.spv",
		"Assets/Shaders/54_ThreadedRendering/obj.frag.spv"
	);

	m_ParticleTexture = DVKTexture::Create2D(
		"Assets/textures/flare3.png",
		m_VulkanDevice,
		cmdBuffer
	);

	m_ParticleMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_ParticleShader
	);
	m_ParticleMaterial->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_ParticleMaterial->pipelineInfo.rasterizationState.polygonMode = VK_POLYGON_MODE_FILL;
	m_ParticleMaterial->pipelineInfo.inputAssemblyState.primitiveRestartEnable = VK_FALSE;
	m_ParticleMaterial->pipelineInfo.depthStencilState.depthTestEnable = VK_FALSE;
	m_ParticleMaterial->pipelineInfo.depthStencilState.depthWriteEnable = VK_FALSE;
	m_ParticleMaterial->pipelineInfo.depthStencilState.stencilTestEnable = VK_FALSE;
	m_ParticleMaterial->pipelineInfo.depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].blendEnable = VK_TRUE;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_ParticleMaterial->pipelineInfo.blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
	m_ParticleMaterial->PreparePipeline();
	m_ParticleMaterial->SetTexture("diffuseMap",m_ParticleTexture);

	// particle instance data
	{
		// write instance index
		DVKPrimitive* primitive = m_ParticleModel->meshes[0]->primitives[0];
		primitive->instanceDatas.resize(INSTANCE_COUNT);

		for(int32 i = 0; i<INSTANCE_COUNT; ++i)
		{
			primitive->instanceDatas[i] = i;
		}

		// create instance buffer
		primitive->indexBuffer->instanceCount = INSTANCE_COUNT;
		primitive->instanceBuffer = DVKVertexBuffer::Create(
			m_VulkanDevice,
			cmdBuffer,
			primitive->instanceDatas,
			m_ParticleShader->instancesAttributes
		);
	}

	delete cmdBuffer;
}

void ThreadedRenderingDemo::DestroyAssets()
{
	m_ThreadRunning = false;
	vkQueueWaitIdle(m_VulkanDevice->GetPresentQueue()->GetHandle());
	m_FrameStartCV.notify_all();

	delete m_RoleModel;
	delete m_ParticleModel;
	delete m_ParticleShader;
	delete m_ParticleMaterial;
	delete m_ParticleTexture;

	for(int32 i = 0; i<m_Particles.size(); ++i)
	{
		delete m_Particles[i];
	}
	m_Particles.clear();

	for(int32 i = 0; i<m_UICommandBuffers.size(); ++i)
	{
		delete m_UICommandBuffers[i];
	}

	for(int32 i = 0; i<m_ThreadDatas.size(); ++i)
	{
		for(int32 j = 0; j<m_ThreadDatas[i]->threadCommandBuffers.size(); ++j)
		{
			delete m_ThreadDatas[i]->threadCommandBuffers[j];
		}

		vkDestroyCommandPool(m_VulkanDevice->GetInstanceHandle(),m_ThreadDatas[i]->commandPool,VULKAN_CPU_ALLOCATOR);
		delete m_ThreadDatas[i];
	}
	m_ThreadDatas.clear();

	for(int32 i = 0; i<m_Threads.size(); ++i)
	{
		delete m_Threads[i];
	}
	m_Threads.clear();
}

void ThreadedRenderingDemo::SetupCommandBuffers(int32 backBufferIndex)
{
	float w = m_FrameWidth;
	float h = m_FrameHeight;
	float tx = 0;
	float ty = 0;

	VkViewport viewport = { };
	viewport.x = tx;
	viewport.y = m_FrameHeight-ty;
	viewport.width = w;
	viewport.height = -h;    // flip y axis
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.extent.width = w;
	scissor.extent.height = h;
	scissor.offset.x = tx;
	scissor.offset.y = ty;

	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo;
	ZeroVulkanStruct(cmdBufferInheritanceInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);
	cmdBufferInheritanceInfo.renderPass = m_RenderPass;
	cmdBufferInheritanceInfo.framebuffer = m_FrameBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	VkClearValue clearValues[2];
	clearValues[0].color = {
		{ 0.2f,0.2f,0.2f,1.0f }
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

	vkCmdBeginRenderPass(commandBuffer,&renderPassBeginInfo,VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

	RenderUI(cmdBufferInheritanceInfo,backBufferIndex);

	for(int32 i = 0; i<m_ThreadDatas.size(); ++i)
	{
		vkCmdExecuteCommands(commandBuffer,1,&(m_ThreadDatas[i]->threadCommandBuffers[backBufferIndex]->cmdBuffer));
	}
	vkCmdExecuteCommands(commandBuffer,1,&(m_UICommandBuffers[backBufferIndex]->cmdBuffer));

	vkCmdEndRenderPass(commandBuffer);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void ThreadedRenderingDemo::RenderUI(VkCommandBufferInheritanceInfo inheritanceInfo,int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_UICommandBuffers[backBufferIndex]->cmdBuffer;

	VkCommandBufferBeginInfo cmdBufferBeginInfo;
	ZeroVulkanStruct(cmdBufferBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
	cmdBufferBeginInfo.pInheritanceInfo = &inheritanceInfo;

	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBufferBeginInfo));

	float w = m_FrameWidth;
	float h = m_FrameHeight;
	float tx = 0;
	float ty = 0;

	VkViewport viewport = { };
	viewport.x = tx;
	viewport.y = m_FrameHeight-ty;
	viewport.width = w;
	viewport.height = -h;    // flip y axis
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.extent.width = w;
	scissor.extent.height = h;
	scissor.offset.x = tx;
	scissor.offset.y = ty;

	vkCmdSetViewport(commandBuffer,0,1,&viewport);
	vkCmdSetScissor(commandBuffer,0,1,&scissor);

	// ui pass
	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void ThreadedRenderingDemo::InitParmas()
{
	DVKBoundingBox bounds = m_RoleModel->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;
	boundCenter.z -= boundSize.Size()*1.5f;

	m_ViewCamera.SetPosition(boundCenter);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,1500.0f);

	m_UICommandBuffers.resize(GetVulkanRHI()->GetSwapChain()->GetBackBufferCount());
	for(int32 i = 0; i<m_UICommandBuffers.size(); ++i)
	{
		m_UICommandBuffers[i] = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool,VK_COMMAND_BUFFER_LEVEL_SECONDARY);
	}
}

void ThreadedRenderingDemo::InitThreads()
{
	int32 numThreads = std::thread::hardware_concurrency();
	if(numThreads==0)
	{
		numThreads = 8;
	}

	if(numThreads>8)
	{
		numThreads = 8;
	}

	DVKPrimitive* primitive = m_RoleModel->meshes[0]->primitives[0];

	int32 threadNum = numThreads*35;
	int32 perThread = primitive->vertexCount/threadNum;
	int32 remainNum = primitive->vertexCount-perThread*threadNum;
	int32 dataIndex = 0;

	m_Particles.resize(threadNum);
	for(int32 i = 0; i<threadNum; ++i)
	{
		int32 count = remainNum>0 ? perThread+1 : perThread;
		remainNum -= 1;

		m_Particles[i] = new ParticleModel(m_ParticleModel,m_ParticleMaterial,m_RoleModel,dataIndex,count);

		dataIndex += count;
	}

	// thread task
	m_MainFrameID = 0;
	m_ThreadRunning = true;

	perThread = m_Particles.size()/numThreads;
	remainNum = m_Particles.size()-perThread*numThreads;
	dataIndex = 0;

	m_ThreadDatas.resize(numThreads);
	m_Threads.resize(numThreads);

	for(int32 i = 0; i<numThreads; ++i)
	{
		// prepare thread data
		m_ThreadDatas[i] = new ThreadData();

		// thread particles
		int32 count = remainNum>0 ? perThread+1 : perThread;
		remainNum -= 1;

		for(int32 index = dataIndex; index<dataIndex+count; ++index)
		{
			m_ThreadDatas[i]->particles.push_back(m_Particles[index]);
		}

		dataIndex += count;

		// command pool per thread
		VkCommandPoolCreateInfo cmdPoolInfo;
		ZeroVulkanStruct(cmdPoolInfo,VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
		cmdPoolInfo.queueFamilyIndex = GetVulkanRHI()->GetDevice()->GetPresentQueue()->GetFamilyIndex();
		cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		VERIFYVULKANRESULT(vkCreateCommandPool(m_VulkanDevice->GetInstanceHandle(),&cmdPoolInfo,VULKAN_CPU_ALLOCATOR,&(m_ThreadDatas[i]->commandPool)));

		// command buffers per frame
		m_ThreadDatas[i]->threadCommandBuffers.resize(GetVulkanRHI()->GetSwapChain()->GetBackBufferCount());
		for(int32 index = 0; index<m_ThreadDatas[i]->threadCommandBuffers.size(); ++index)
		{
			m_ThreadDatas[i]->threadCommandBuffers[index] = DVKCommandBuffer::Create(m_VulkanDevice,m_ThreadDatas[i]->commandPool,VK_COMMAND_BUFFER_LEVEL_SECONDARY);
		}

		// start thread
		m_ThreadDatas[i]->index = i;
		m_Threads[i] = new MyThread(
			[=]
		{
			ThreadRendering(m_ThreadDatas[i]);
		}
		);
	}
}

void ThreadedRenderingDemo::ThreadRendering(void* param)
{
	ThreadData* threadData = (ThreadData*)param;
	threadData->frameID = 0;

	while(true)
	{
		{
			std::unique_lock<std::mutex> guardLock(m_FrameStartLock);
			if(threadData->frameID==m_MainFrameID)
			{
				m_FrameStartCV.wait(guardLock);
			}
		}

		threadData->frameID = m_MainFrameID;

		if(!m_ThreadRunning)
		{
			break;
		}

		// update particles
		for(int32 i = 0; i<threadData->particles.size(); ++i)
		{
			threadData->particles[i]->Update(m_BonesData,m_ViewCamera,m_FrameTime,m_FrameDelta);
		}

		// record commands
		VkCommandBuffer commandBuffer = threadData->threadCommandBuffers[m_bufferIndex]->cmdBuffer;

		VkCommandBufferInheritanceInfo cmdBufferInheritanceInfo;
		ZeroVulkanStruct(cmdBufferInheritanceInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);
		cmdBufferInheritanceInfo.renderPass = m_RenderPass;
		cmdBufferInheritanceInfo.framebuffer = m_FrameBuffers[m_bufferIndex];

		VkCommandBufferBeginInfo cmdBufferBeginInfo;
		ZeroVulkanStruct(cmdBufferBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
		cmdBufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;
		cmdBufferBeginInfo.pInheritanceInfo = &cmdBufferInheritanceInfo;

		VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBufferBeginInfo));

		float w = m_FrameWidth;
		float h = m_FrameHeight;
		float tx = 0;
		float ty = 0;

		VkViewport viewport = { };
		viewport.x = tx;
		viewport.y = m_FrameHeight-ty;
		viewport.width = w;
		viewport.height = -h;    // flip y axis
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = { };
		scissor.extent.width = w;
		scissor.extent.height = h;
		scissor.offset.x = tx;
		scissor.offset.y = ty;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		for(int32 i = 0; i<threadData->particles.size(); ++i)
		{
			threadData->particles[i]->Draw(commandBuffer,m_ViewCamera);
		}

		VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));

		// notify thread done
		{
			std::lock_guard<std::mutex> lockGuard(m_ThreadDoneLock);
			m_ThreadDoneCount += 1;
			m_ThreadDoneCV.notify_one();
		}
	}

	{
		std::lock_guard<std::mutex> lockGuard(writeMutex);
		MLOG("Thread exist -> index = %d",threadData->index);
	}
}

void ThreadedRenderingDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void ThreadedRenderingDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}