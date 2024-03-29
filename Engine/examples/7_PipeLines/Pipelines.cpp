#include "Pipelines.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<PipeLines>(1400,900,"LoadMesh",cmdLine);
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
	bool hovered=UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}
	UpdateUniformBuffers(time,delta);
	auto bufferindex=AcquireBackbufferIndex();
	Present(bufferindex);
}

void PipeLines::UpdateUniformBuffers(float time,float delta)
{

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
	m_GUI->EndFrame();
	return false;
}

void PipeLines::CreateGUI()
{
	if(!m_GUI)
	{
		m_GUI = new ImageGUIContext();
		m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
	}
}

void PipeLines::DestroyGUI()
{
	if(m_GUI)
	{
		m_GUI->Destroy();
		delete m_GUI;
		m_GUI = nullptr;
	}
}

void PipeLines::LoadAssets()
{
	auto cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);
	m_Model = DVKModel::LoadFromFile("Assets/models/suzanne.obj",m_VulkanDevice,cmdBuffer,{ VertexAttribute::VA_Position,VertexAttribute::VA_Normal });
	delete cmdBuffer;
}

void PipeLines::DestroyAssets()
{
	if(m_Model)
	{
		delete m_Model;
		m_Model = nullptr;
	}
}

void PipeLines::CreateUniformBuffers()
{

}

void PipeLines::CreateDescriptorSetLayout()
{

}

void PipeLines::CreateDescriptorSet()
{

}

void PipeLines::CreatePipelines()
{

}

void PipeLines::SetupCommandBuffers()
{

}

void PipeLines::DestroyDescriptorSetLayout()
{

}

void PipeLines::DestroyPipelines()
{

}

void PipeLines::DestroyUniformBuffers()
{

}
