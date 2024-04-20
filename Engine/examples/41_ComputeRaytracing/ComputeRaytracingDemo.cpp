#include "ComputeRaytracingDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<ComputeRaytracingDemo>(1400,900,"ComputeRaytracingDemo",cmdLine);
}

ComputeRaytracingDemo::ComputeRaytracingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

ComputeRaytracingDemo::~ComputeRaytracingDemo()
{

}

bool ComputeRaytracingDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	LoadAssets();
	InitParmas();
	SetupComputeCommand();

	m_Ready = true;

	return true;
}

void ComputeRaytracingDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void ComputeRaytracingDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void ComputeRaytracingDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);
	UpdateUI(time,delta);

	SetupGfxCommand(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool ComputeRaytracingDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("ComputeRaytracingDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void ComputeRaytracingDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// fullscreen
	m_Quad = DVKDefaultRes::fullQuad;

	// fullscreen shader
	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/44_ComputeRaytracing/Texture.vert.spv",
		"Assets/Shaders/44_ComputeRaytracing/Texture.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_Material->PreparePipeline();

	// prepare scene data
	m_SceneModel = DVKModel::LoadFromFile(
		"Assets/models/simplescene.obj",
		m_VulkanDevice,
		nullptr,
		{
			VertexAttribute::VA_Position
		}
	);

	// copy scene data to storage buffer
	int32 count = 0;
	std::vector<float> bufferDatas;
	// vec4 datas[0] = count
	bufferDatas.push_back(0);
	bufferDatas.push_back(0);
	bufferDatas.push_back(0);
	bufferDatas.push_back(0);

	Vector4 diffuses[10] = {
		Vector4(0.75f,0.75f,0.75f,1), // plane
		Vector4(0.75f,0.75f,0.75f,1), // plane
		Vector4(1,1,1,1), // diamond
		Vector4(0.75f,0.75f,0.75f,1), // plane
		Vector4(0.75f,0.75f,0.75f,1), // plane
		Vector4(1,0,1,1), // diamond
		Vector4(0.75f,0.75f,0.75f,1), // plane
		Vector4(0.75f,0.75f,0.75f,1), // plane
		Vector4(0,0,1,1), // diamond
		Vector4(0.2f,1,0.2f,1) // trillion
	};

	for(int32 meshID = 0; meshID<m_SceneModel->meshes.size(); ++meshID)
	{
		auto mesh = m_SceneModel->meshes[meshID];
		auto name = mesh->linkNode->name;

		Material& material = m_RaytracingParam.materials[meshID];
		material.ambientColor = Vector4(0.1f,0.1f,0.1f,0.1f);
		material.specularColor = Vector4(1.0f,1.0f,1.0f,40.0f);
		material.diffuseColor = diffuses[meshID];
		material.reflectedColor = diffuses[meshID];
		material.refractedColor = diffuses[meshID];
		material.refractiveIndex = Vector4(2.407f,2.407f,2.407f,0.0f);

		if(name.find("diamond",0)!=std::string::npos)
		{
			material.reflectedColor.w = 0.0f;
			material.refractedColor.w = 1.0f;
		}
		else if(name.find("Plane",0)!=std::string::npos)
		{
			material.reflectedColor.w = 0.5f;
			material.refractedColor.w = 0.0f;
		}
		else if(name.find("Trillion",0)!=std::string::npos)
		{
			material.reflectedColor.w = 0.0f;
			material.refractedColor.w = 1.0f;
		}

		for(int32 primitiveID = 0; primitiveID<mesh->primitives.size(); ++primitiveID)
		{
			count += 1;

			auto primitive = mesh->primitives[primitiveID];

			bufferDatas.push_back(meshID);
			bufferDatas.push_back(primitiveID);
			bufferDatas.push_back(primitive->vertexCount);
			bufferDatas.push_back(primitive->triangleNum);

			for(int32 i = 0; i<primitive->triangleNum; ++i)
			{
				bufferDatas.push_back(primitive->indices[i*3+0]);
				bufferDatas.push_back(primitive->indices[i*3+1]);
				bufferDatas.push_back(primitive->indices[i*3+2]);
				bufferDatas.push_back(0);
			}
			for(int32 i = 0; i<primitive->vertexCount; ++i)
			{
				bufferDatas.push_back(primitive->vertices[i*3+0]);
				bufferDatas.push_back(primitive->vertices[i*3+1]);
				bufferDatas.push_back(primitive->vertices[i*3+2]);
				bufferDatas.push_back(0);
			}
		}
	}
	bufferDatas[0] = count;

	// upload scene data to storage buffer
	DVKBuffer* stagingBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		bufferDatas.size()*sizeof(float),
		bufferDatas.data()
	);

	m_SceneBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		bufferDatas.size()*sizeof(float)
	);

	cmdBuffer->Begin();

	VkBufferCopy copyRegion = { };
	copyRegion.size = bufferDatas.size()*sizeof(float);
	vkCmdCopyBuffer(cmdBuffer->cmdBuffer,stagingBuffer->buffer,m_SceneBuffer->buffer,1,&copyRegion);

	cmdBuffer->End();
	cmdBuffer->Submit();

	delete stagingBuffer;

	// compute resources
	// create target image
	m_ComputeTarget = DVKTexture::Create2D(
		m_VulkanDevice,
		cmdBuffer,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		1024,
		512,
		VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		ImageLayoutBarrier::ComputeGeneralRW
	);

	m_ComputeShader = DVKShader::Create(
		m_VulkanDevice,
		"Assets/Shaders/44_ComputeRaytracing/Raytracing.comp.spv"
	);
	m_ComputeProcessor = DVKCompute::Create(m_VulkanDevice,m_PipelineCache,m_ComputeShader);
	m_ComputeProcessor->SetStorageTexture("outputImage",m_ComputeTarget);
	m_ComputeProcessor->SetStorageBuffer("inSceneData",m_SceneBuffer);

	// bind compute output texture
	m_Material->SetTexture("diffuseMap",m_ComputeTarget);

	// compute command
	m_ComputeCommand = DVKCommandBuffer::Create(
		m_VulkanDevice,
		m_ComputeCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		m_VulkanDevice->GetComputeQueue()
	);

	delete cmdBuffer;
}

void ComputeRaytracingDemo::DestroyAssets()
{
	delete m_ComputeShader;
	delete m_ComputeTarget;
	delete m_ComputeProcessor;
	delete m_ComputeCommand;

	delete m_Material;
	delete m_Shader;

	delete m_SceneModel;
	delete m_SceneBuffer;
}

void ComputeRaytracingDemo::SetupComputeCommand()
{
	m_ComputeCommand->Begin();

	m_ComputeProcessor->SetUniform("uboParam",&m_RaytracingParam,sizeof(RaytracingParamBlock));
	m_ComputeProcessor->BindDispatch(m_ComputeCommand->cmdBuffer,m_ComputeTarget->width/16,m_ComputeTarget->height/16,1);

	m_ComputeCommand->Submit();
}

void ComputeRaytracingDemo::SetupGfxCommand(int32 backBufferIndex)
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
	vkCmdBeginRenderPass(commandBuffer,&renderPassBeginInfo,VK_SUBPASS_CONTENTS_INLINE);

	vkCmdSetViewport(commandBuffer,0,1,&viewport);
	vkCmdSetScissor(commandBuffer,0,1,&scissor);

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());

	m_Material->BeginFrame();
	m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_Quad->meshes[0]->BindDrawCmd(commandBuffer);
	m_Material->EndFrame();

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);
	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void ComputeRaytracingDemo::InitParmas()
{
	DVKCamera camera;
	camera.SetPosition(0,2.5f,-10.0f);
	camera.LookAt(0,2.5f,0);
	camera.Perspective(PI/4,m_ComputeTarget->width,m_ComputeTarget->height,1.0f,1500.0f);

	m_RaytracingParam.invView = camera.GetView();
	m_RaytracingParam.invView.SetInverse();

	m_RaytracingParam.invProjection = camera.GetProjection();
	m_RaytracingParam.invProjection.SetInverse();

	m_RaytracingParam.lightPos = Vector4(0.0f,5.0f,0.0f,8.5f);
	m_RaytracingParam.cameraPos = camera.GetTransform().GetOrigin();
}

void ComputeRaytracingDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void ComputeRaytracingDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}