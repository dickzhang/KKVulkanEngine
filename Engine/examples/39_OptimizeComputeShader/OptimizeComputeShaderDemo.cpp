#include "OptimizeComputeShaderDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<OptimizeComputeShaderDemo>(1400,900,"OptimizeComputeShaderDemo",cmdLine);
}

OptimizeComputeShaderDemo::OptimizeComputeShaderDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

OptimizeComputeShaderDemo::~OptimizeComputeShaderDemo()
{

}
 bool OptimizeComputeShaderDemo::Init() 
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	LoadAssets();
	ProcessImage();

	m_Ready = true;

	return true;
}

 void OptimizeComputeShaderDemo::Exist() 
{
	ModuleBase::Release();

	DestroyAssets();
	DestroyGUI();
}

 void OptimizeComputeShaderDemo::Loop(float time,float delta) 
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}


void OptimizeComputeShaderDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);
	bool hovered = UpdateUI(time,delta);

	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	SetupCommandBuffers(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool OptimizeComputeShaderDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("OptimizeComputeShaderDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		if(ImGui::Combo("Filter",&m_FilterIndex,m_FilterNames.data(),m_FilterNames.size()))
		{
			if(m_FilterIndex==0)
			{
				m_Material->SetTexture("diffuseMap",m_Texture);
			}
			else
			{
				m_Material->SetTexture("diffuseMap",m_ComputeTargets[m_FilterIndex-1]);
			}
		}

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void OptimizeComputeShaderDemo::ProcessImage()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(
		m_VulkanDevice,
		m_ComputeCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		m_VulkanDevice->GetComputeQueue()
	);

	// create target image
	for(int32 i = 0; i<3; ++i)
	{
		m_ComputeTargets[i] = DVKTexture::Create2D(
			m_VulkanDevice,
			cmdBuffer,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_IMAGE_ASPECT_COLOR_BIT,
			m_Texture->width,
			m_Texture->height,
			VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT,
			VK_SAMPLE_COUNT_1_BIT,
			ImageLayoutBarrier::ComputeGeneralRW
		);
	}

	const char* shaderNames[3] = {
		"assets/shaders/42_OptimizeComputeShader/Contrast.comp.spv",
		"assets/shaders/42_OptimizeComputeShader/Gamma.comp.spv",
		"assets/shaders/42_OptimizeComputeShader/ColorInvert.comp.spv",
	};

	for(int32 i = 0; i<3; ++i)
	{
		m_ComputeShaders[i] = DVKShader::Create(m_VulkanDevice,shaderNames[i]);
		m_ComputeProcessors[i] = DVKCompute::Create(m_VulkanDevice,m_PipelineCache,m_ComputeShaders[i]);
		m_ComputeProcessors[i]->SetStorageTexture("inputImage",m_Texture);
		m_ComputeProcessors[i]->SetStorageTexture("outputImage",m_ComputeTargets[i]);
	}

	// compute command
	cmdBuffer->Begin();

	for(int32 i = 0; i<3; ++i)
	{
		m_ComputeProcessors[i]->BindDispatch(cmdBuffer->cmdBuffer,m_ComputeTargets[i]->width/16,m_ComputeTargets[i]->height/16,1);
	}

	cmdBuffer->End();
	cmdBuffer->Submit();

	m_FilterIndex = 0;
	m_FilterNames.resize(4);
	m_FilterNames[0] = "Original";
	m_FilterNames[1] = "Contrast";
	m_FilterNames[2] = "Gamma";
	m_FilterNames[3] = "ColorInvert";

	delete cmdBuffer;
}

void OptimizeComputeShaderDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_ModelPlane = DVKModel::LoadFromFile(
		"assets/models/plane_z.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0
		}
	);
	m_ModelPlane->rootNode->localMatrix.AppendScale(Vector3(2,1,1));

	m_Texture = DVKTexture::Create2D(
		"assets/textures/game0.jpg",
		m_VulkanDevice,
		cmdBuffer,
		VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_STORAGE_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		ImageLayoutBarrier::ComputeGeneralRW
	);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"assets/shaders/42_OptimizeComputeShader/Texture.vert.spv",
		"assets/shaders/42_OptimizeComputeShader/Texture.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_Material->PreparePipeline();
	m_Material->SetTexture("diffuseMap",m_Texture);

	delete cmdBuffer;
}

void OptimizeComputeShaderDemo::DestroyAssets()
{
	for(int32 i = 0; i<3; ++i)
	{
		delete m_ComputeShaders[i];
		delete m_ComputeTargets[i];
		delete m_ComputeProcessors[i];
	}

	delete m_ModelPlane;
	delete m_Texture;

	delete m_Material;
	delete m_Shader;
}

void OptimizeComputeShaderDemo::SetupCommandBuffers(int32 backBufferIndex)
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
	for(int32 i = 0; i<m_ModelPlane->meshes.size(); ++i)
	{
		m_MVPParam.model = m_ModelPlane->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Material->BeginObject();
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->EndObject();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,i);
		m_ModelPlane->meshes[i]->BindDrawCmd(commandBuffer);
	}
	m_Material->EndFrame();

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);
	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void OptimizeComputeShaderDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,0,-2.5f);
	m_ViewCamera.LookAt(0,0,0);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,1500.0f);
}

void OptimizeComputeShaderDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("assets/fonts/Ubuntu-Regular.ttf");
}

void OptimizeComputeShaderDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}