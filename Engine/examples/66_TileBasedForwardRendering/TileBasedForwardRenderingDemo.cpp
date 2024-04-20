#include "TileBasedForwardRenderingDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<TileBasedForwardRenderingDemo>(1400,900,"TileBasedForwardRenderingDemo",cmdLine);
}

TileBasedForwardRenderingDemo::TileBasedForwardRenderingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

TileBasedForwardRenderingDemo::~TileBasedForwardRenderingDemo()
{

}

bool TileBasedForwardRenderingDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	LoadAssets();

	m_Ready = true;
	return true;
}

void TileBasedForwardRenderingDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void TileBasedForwardRenderingDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void TileBasedForwardRenderingDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	UpdateLights(time,delta);

	m_CullingParam.invViewProj = m_ViewCamera.GetViewProjection();
	m_CullingParam.invViewProj.SetInverse();

	m_CullingParam.pos = m_ViewCamera.GetTransform().GetOrigin();

	SetupCommandBuffers(bufferIndex);
	ModuleBase::Present(bufferIndex);
}

void TileBasedForwardRenderingDemo::UpdateLights(float time,float delta)
{
	DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
	Vector3 extend = bounds.max-bounds.min;
	float size = MMath::Min(extend.x,MMath::Min(extend.y,extend.z));

	for(int32 i = 0; i<m_LightParam.count.x; ++i)
	{
		PointLight& light = m_LightParam.lights[i];

		light.position = m_LightInfo.position[i]+m_LightInfo.direction[i]*MMath::Cos(time)*size;
	}
}

bool TileBasedForwardRenderingDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("TileBasedForwardRenderingDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		int index = m_Debug.x;
		ImGui::Combo("Debug",&index,"None\0Normal\0\Tile\0");
		m_Debug.x = index;

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void TileBasedForwardRenderingDemo::InitLights()
{
	m_LightParam.count.Set(LIGHT_SIZE,LIGHT_SIZE,LIGHT_SIZE,LIGHT_SIZE);

	DVKBoundingBox bounds = m_Model->rootNode->GetBounds();

	for(int32 i = 0; i<LIGHT_SIZE; ++i)
	{
		Vector3 position;
		position.x = MMath::FRandRange(bounds.min.x,bounds.max.x);
		position.y = MMath::FRandRange(bounds.min.y,bounds.max.y);
		position.z = MMath::FRandRange(bounds.min.z,bounds.max.z);

		float radius = MMath::FRandRange(0.5f,2.5f);

		Vector3 color;
		color.x = MMath::FRandRange(0.0f,2.5f);
		color.y = MMath::FRandRange(0.0f,2.5f);
		color.z = MMath::FRandRange(0.0f,2.5f);

		PointLight light(position,radius,color);

		m_LightParam.lights[i] = light;

		m_LightInfo.direction[i] = Vector3(
			MMath::FRandRange(-1.0f,1.0f),
			MMath::FRandRange(-1.0f,1.0f),
			MMath::FRandRange(-1.0f,1.0f)
		);
		m_LightInfo.direction[i].Normalize();

		m_LightInfo.position[i] = position;
	}

	m_TileCountPerRow = (m_FrameWidth-1)/TILE_SIZE+1;
	m_TileCountPerCol = (m_FrameHeight-1)/TILE_SIZE+1;

	m_CullingParam.frameSize.x = m_FrameWidth;
	m_CullingParam.frameSize.y = m_FrameHeight;

	m_CullingParam.tileNum.x = m_TileCountPerRow;
	m_CullingParam.tileNum.y = m_TileCountPerCol;
}

void TileBasedForwardRenderingDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// scene
	m_Model = DVKModel::LoadFromFile(
		"Assets/models/scene1.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0,
			VertexAttribute::VA_Normal
		}
	);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/69_TileBasedForwardRendering/obj.vert.spv",
		"Assets/Shaders/69_TileBasedForwardRendering/obj.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_Material->PreparePipeline();

	// lights
	InitLights();

	// depth
	m_PreDepthTexture = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(m_DepthFormat,false),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	DVKRenderPassInfo passInfo(m_PreDepthTexture,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE);
	m_PreDepthRTT = DVKRenderTarget::Create(m_VulkanDevice,passInfo);

	m_DepthShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/69_TileBasedForwardRendering/depth.vert.spv",
		"Assets/Shaders/69_TileBasedForwardRendering/depth.frag.spv"
	);

	m_DepthMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_PreDepthRTT,
		m_PipelineCache,
		m_DepthShader
	);
	m_DepthMaterial->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_DepthMaterial->pipelineInfo.colorAttachmentCount = 0;
	m_DepthMaterial->PreparePipeline();

	// tiles
	m_LightsCullingBuffer = DVKBuffer::CreateBuffer(
		m_VulkanDevice,
		VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		sizeof(LightVisiblity)*m_TileCountPerRow*m_TileCountPerCol
	);

	m_ComputeShader = DVKShader::Create(
		m_VulkanDevice,
		"Assets/Shaders/69_TileBasedForwardRendering/lightCulling.comp.spv"
	);

	m_ComputeProcessor = DVKCompute::Create(
		m_VulkanDevice,
		m_PipelineCache,
		m_ComputeShader
	);
	m_ComputeProcessor->SetStorageBuffer("lightsCullingBuffer",m_LightsCullingBuffer);
	m_ComputeProcessor->SetTexture("depthTexture",m_PreDepthTexture);

	m_Material->SetStorageBuffer("lightsCullingBuffer",m_LightsCullingBuffer);

	delete cmdBuffer;
}

void TileBasedForwardRenderingDemo::DestroyAssets()
{
	delete m_Model;
	delete m_Shader;
	delete m_Material;

	delete m_DepthShader;
	delete m_DepthMaterial;
	delete m_PreDepthTexture;
	delete m_PreDepthRTT;

	delete m_ComputeShader;
	delete m_ComputeProcessor;
	delete m_LightsCullingBuffer;
}

void TileBasedForwardRenderingDemo::PreDepthPass(int backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	m_PreDepthRTT->BeginRenderPass(commandBuffer);

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DepthMaterial->GetPipeline());
	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DepthMaterial->GetPipeline());

		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_DepthMaterial->BeginFrame();
		m_DepthMaterial->BeginObject();
		m_DepthMaterial->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_DepthMaterial->EndObject();
		m_DepthMaterial->EndFrame();

		m_DepthMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);
	}

	m_PreDepthRTT->EndRenderPass(commandBuffer);
}

void TileBasedForwardRenderingDemo::FinnalPass(int backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

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

	vkCmdSetViewport(commandBuffer,0,1,&viewport);
	vkCmdSetScissor(commandBuffer,0,1,&scissor);

	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());

		m_Material->BeginFrame();

		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Material->BeginObject();
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->SetLocalUniform("uboLights",&m_LightParam,sizeof(LightsParamBlock));
		m_Material->SetLocalUniform("uboCulling",&m_CullingParam,sizeof(CullingParamBlock));
		m_Material->SetLocalUniform("uboDebug",&m_Debug,sizeof(Vector4));
		m_Material->EndObject();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);

		m_Material->EndFrame();
	}

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
}

void TileBasedForwardRenderingDemo::SetupComputeCommand(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkBufferMemoryBarrier bufferBarrier;
	ZeroVulkanStruct(bufferBarrier,VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
	bufferBarrier.buffer = m_LightsCullingBuffer->buffer;
	bufferBarrier.size = m_LightsCullingBuffer->size;
	bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
	bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	bufferBarrier.srcQueueFamilyIndex = m_VulkanDevice->GetGraphicsQueue()->GetFamilyIndex();
	bufferBarrier.dstQueueFamilyIndex = m_VulkanDevice->GetComputeQueue()->GetFamilyIndex();

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		0,
		0,
		nullptr,
		1,
		&bufferBarrier,
		0,
		nullptr
	);

	m_ComputeProcessor->SetUniform("uboCulling",&m_CullingParam,sizeof(CullingParamBlock));
	m_ComputeProcessor->SetUniform("uboLights",&m_LightParam,sizeof(LightsParamBlock));
	m_ComputeProcessor->BindDispatch(commandBuffer,m_TileCountPerRow,m_TileCountPerCol,1);

	bufferBarrier.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
	bufferBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	bufferBarrier.srcQueueFamilyIndex = m_VulkanDevice->GetComputeQueue()->GetFamilyIndex();
	bufferBarrier.dstQueueFamilyIndex = m_VulkanDevice->GetGraphicsQueue()->GetFamilyIndex();

	vkCmdPipelineBarrier(
		commandBuffer,
		VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
		0,
		0,
		nullptr,
		1,
		&bufferBarrier,
		0,
		nullptr
	);
}

void TileBasedForwardRenderingDemo::SetupCommandBuffers(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	PreDepthPass(backBufferIndex);
	SetupComputeCommand(backBufferIndex);
	FinnalPass(backBufferIndex);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void TileBasedForwardRenderingDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,2.5f,-20.0f);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,3000.0f);
	m_ViewCamera.speedFactor = 2.0f;
	m_ViewCamera.smooth = 0.5f;
}

void TileBasedForwardRenderingDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void TileBasedForwardRenderingDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}