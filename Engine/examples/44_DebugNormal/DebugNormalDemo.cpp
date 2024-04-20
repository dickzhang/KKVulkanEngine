#include "DebugNormalDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<DebugNormalDemo>(1400,900,"DebugNormalDemo",cmdLine);
}

DebugNormalDemo::DebugNormalDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

DebugNormalDemo::~DebugNormalDemo()
{

}

bool DebugNormalDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	LoadAssets();

	m_Ready = true;

	return true;
}

void DebugNormalDemo::Exist()
{
	ModuleBase::Release();

	DestroyAssets();
	DestroyGUI();
}

void DebugNormalDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void DebugNormalDemo::Draw(float time,float delta)
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

bool DebugNormalDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("DebugNormalDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void DebugNormalDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_Model = DVKModel::LoadFromFile(
		"Assets/models/suzanne.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_Normal
		}
	);
	m_Model->rootNode->localMatrix.AppendRotation(180,Vector3::UpVector);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/47_DebugNormal/Normal.vert.spv",
		"Assets/Shaders/47_DebugNormal/Normal.frag.spv",
		nullptr
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();

	m_LineShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/47_DebugNormal/Line.vert.spv",
		"Assets/Shaders/47_DebugNormal/Line.frag.spv",
		"Assets/Shaders/47_DebugNormal/Line.geom.spv"
	);

	m_LineMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_LineShader
	);
	m_LineMaterial->PreparePipeline();

	delete cmdBuffer;
}

void DebugNormalDemo::DestroyAssets()
{
	delete m_Model;

	delete m_Material;
	delete m_Shader;

	delete m_LineMaterial;
	delete m_LineShader;
}

void DebugNormalDemo::SetupCommandBuffers(int32 backBufferIndex)
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
	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Material->BeginObject();
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->EndObject();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,i);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);
	}
	m_Material->EndFrame();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_LineMaterial->GetPipeline());
	m_LineMaterial->BeginFrame();
	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_LineMaterial->BeginObject();
		m_LineMaterial->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_LineMaterial->EndObject();

		m_LineMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,i);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);
	}
	m_LineMaterial->EndFrame();

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);
	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void DebugNormalDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,0,-50.0f);
	m_ViewCamera.LookAt(0,0,0);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,500.0f);
}

void DebugNormalDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void DebugNormalDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}