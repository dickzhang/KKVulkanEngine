#include "QueryStatisticsDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<QueryStatisticsDemo>(1400,900,"QueryStatisticsDemo",cmdLine);
}

QueryStatisticsDemo::QueryStatisticsDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

QueryStatisticsDemo::~QueryStatisticsDemo()
{

}

bool QueryStatisticsDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	LoadAssets();

	m_Ready = true;

	return true;
}

void QueryStatisticsDemo::Exist()
{
	ModuleBase::Release();

	DestroyAssets();
	DestroyGUI();
}

void QueryStatisticsDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void QueryStatisticsDemo::Draw(float time,float delta)
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

	vkGetQueryPoolResults(
		m_Device,
		m_QueryPool,
		0,
		1,
		sizeof(uint64)*QUERY_STATS_COUNT,
		m_QueryStats,
		sizeof(uint64),
		VK_QUERY_RESULT_64_BIT
	);
}

bool QueryStatisticsDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("QueryStatisticsDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		for(int32 i = 0; i<m_StatNames.size(); ++i)
		{
			ImGui::Text("%s : %d",m_StatNames[i],m_QueryStats[i]);
		}

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void QueryStatisticsDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	VkQueryPoolCreateInfo queryPoolCreateInfo;
	ZeroVulkanStruct(queryPoolCreateInfo,VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO);
	queryPoolCreateInfo.queryType = VK_QUERY_TYPE_PIPELINE_STATISTICS;
	queryPoolCreateInfo.queryCount = QUERY_STATS_COUNT;
	queryPoolCreateInfo.pipelineStatistics =
		VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_VERTICES_BIT|
		VK_QUERY_PIPELINE_STATISTIC_INPUT_ASSEMBLY_PRIMITIVES_BIT|
		VK_QUERY_PIPELINE_STATISTIC_VERTEX_SHADER_INVOCATIONS_BIT|
		VK_QUERY_PIPELINE_STATISTIC_FRAGMENT_SHADER_INVOCATIONS_BIT|
		VK_QUERY_PIPELINE_STATISTIC_CLIPPING_PRIMITIVES_BIT|
		VK_QUERY_PIPELINE_STATISTIC_CLIPPING_INVOCATIONS_BIT|
		VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_CONTROL_SHADER_PATCHES_BIT|
		VK_QUERY_PIPELINE_STATISTIC_TESSELLATION_EVALUATION_SHADER_INVOCATIONS_BIT;
	VERIFYVULKANRESULT(vkCreateQueryPool(m_Device,&queryPoolCreateInfo,VULKAN_CPU_ALLOCATOR,&m_QueryPool));

	m_StatNames.resize(QUERY_STATS_COUNT);
	m_StatNames[0] = "Vertex count";
	m_StatNames[1] = "Primitives count";
	m_StatNames[2] = "Vert shader invocations";
	m_StatNames[3] = "Clipping invocations";
	m_StatNames[4] = "Clipping primtives";
	m_StatNames[5] = "Frag shader invocations";
	m_StatNames[6] = "Tessellation control shader patches";
	m_StatNames[7] = "Tessellation evaluation shader invocations";

	m_Model = DVKModel::LoadFromFile(
		"Assets/models/simplify_BOTI_Dreamsong_Bridge1.fbx",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_Normal
		}
	);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/40_QueryStatistics/Solid.vert.spv",
		"Assets/Shaders/40_QueryStatistics/Solid.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();

	delete cmdBuffer;
}

void QueryStatisticsDemo::DestroyAssets()
{
	delete m_Model;

	delete m_Material;
	delete m_Shader;

	vkDestroyQueryPool(m_Device,m_QueryPool,VULKAN_CPU_ALLOCATOR);
}

void QueryStatisticsDemo::SetupCommandBuffers(int32 backBufferIndex)
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

	vkCmdResetQueryPool(commandBuffer,m_QueryPool,0,QUERY_STATS_COUNT);

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

	vkCmdBeginQuery(commandBuffer,m_QueryPool,0,0);
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
	vkCmdEndQuery(commandBuffer,m_QueryPool,0);

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);
	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void QueryStatisticsDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,500,-700.0f);
	m_ViewCamera.LookAt(0,250,0);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,1500.0f);

	memset(m_QueryStats,65535,sizeof(uint64)*QUERY_STATS_COUNT);
}

void QueryStatisticsDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void QueryStatisticsDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}