#include "PNTessellationDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<PNTessellationDemo>(1400,900,"PNTessellationDemo",cmdLine);
}

PNTessellationDemo::PNTessellationDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

PNTessellationDemo::~PNTessellationDemo()
{

}
bool PNTessellationDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	LoadAssets();
	InitParmas();

	m_Ready = true;

	return true;
}

void PNTessellationDemo::Exist()
{
	ModuleBase::Release();

	DestroyAssets();
	DestroyGUI();
}

void PNTessellationDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void PNTessellationDemo::Draw(float time,float delta)
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

bool PNTessellationDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("PNTessellationDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::SliderFloat("Level0:",&m_TessParam.level.x,0.1f,5.0f);
		ImGui::SliderFloat("Level1:",&m_TessParam.level.y,0.1f,5.0f);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void PNTessellationDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_Model = DVKModel::LoadFromFile(
		"Assets/models/LizardMage/LizardMage_Lowpoly.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_Normal,
			VertexAttribute::VA_UV0
		}
	);
	m_Model->rootNode->localMatrix.AppendRotation(180,Vector3::UpVector);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/50_PNTessellation/Simple.vert.spv",
		"Assets/Shaders/50_PNTessellation/Simple.frag.spv",
		nullptr,
		nullptr,
		"Assets/Shaders/50_PNTessellation/Simple.tesc.spv",
		"Assets/Shaders/50_PNTessellation/Simple.tese.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_Material->pipelineInfo.rasterizationState.polygonMode = VK_POLYGON_MODE_LINE;
	m_Material->pipelineInfo.inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
	m_Material->pipelineInfo.tessellationState.patchControlPoints = 3;
	m_Material->PreparePipeline();

	delete cmdBuffer;
}

void PNTessellationDemo::DestroyAssets()
{
	delete m_Model;
	delete m_Shader;
	delete m_Material;
}

void PNTessellationDemo::SetupCommandBuffers(int32 backBufferIndex)
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
		m_Material->SetLocalUniform("tessParam",&m_TessParam,sizeof(TessParamBlock));
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->EndObject();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,i);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);
	}
	m_Material->EndFrame();

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);
	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void PNTessellationDemo::InitParmas()
{
	m_TessParam.level.Set(3.0f,1.0f,0.0f,0.0f);

	auto bounds = m_Model->rootNode->GetBounds();
	Vector3 center = (bounds.max+bounds.min)*0.5f;

	m_ViewCamera.SetPosition(center.x,center.y,center.z-10.0f);
	m_ViewCamera.LookAt(center);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,1500.0f);
}

void PNTessellationDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void PNTessellationDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}