#include "SDFFontDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<SDFFontDemo>(1400,900,"SDFFontDemo",cmdLine);
}

SDFFontDemo::SDFFontDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

SDFFontDemo::~SDFFontDemo()
{

}

bool SDFFontDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	LoadAssets();
	InitParmas();
	CreateGUI();

	m_Ready = true;

	return true;
}

void SDFFontDemo::Exist()
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
}

void SDFFontDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void SDFFontDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	m_MVPData.model = m_Model->rootNode->GetGlobalMatrix();
	m_MVPData.view = m_ViewCamera.GetView();
	m_MVPData.projection = m_ViewCamera.GetProjection();

	m_Material->BeginFrame();
	m_Material->BeginObject();
	m_Material->SetLocalUniform("uboMVP",&m_MVPData,sizeof(ModelViewProjectionBlock));
	m_Material->SetLocalUniform("uboSDF",&m_SDFData,sizeof(SDFParamBlock));
	m_Material->EndObject();
	m_Material->EndFrame();

	SetupCommandBuffers(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool SDFFontDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("SDFFontDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		int type = m_SDFData.param2.x;
		ImGui::Combo("Type",&type,"Normal\0Outline\0Glow\0Shadow\0\0");
		m_SDFData.param2.x = type;

		ImGui::ColorEdit4("Main Color",&(m_SDFData.mainColor.x));
		ImGui::SliderFloat("Distance",&(m_SDFData.param0.x),0.0f,1.0f);
		ImGui::SliderFloat("Smooth",&(m_SDFData.param0.w),0.0f,1.0f);

		ImGui::Separator();

		ImGui::ColorEdit4("Outline Color",&(m_SDFData.outlineColor.x));
		ImGui::SliderFloat("Outline Distance",&(m_SDFData.param0.y),0.0f,1.0f);

		ImGui::Separator();

		ImGui::ColorEdit4("Glow Color",&(m_SDFData.glowColor.x));
		ImGui::SliderFloat("Glow Distance",&(m_SDFData.param0.z),0.0f,1.0f);
		ImGui::SliderFloat("Glow Smooth",&(m_SDFData.param1.y),0.0f,1.0f);

		ImGui::Separator();

		ImGui::ColorEdit4("Shadow Color",&(m_SDFData.shadowColor.x));
		ImGui::SliderFloat("Shadow Smooth",&(m_SDFData.param1.x),0.0f,1.0f);
		ImGui::SliderFloat("Shadow Offset X",&(m_SDFData.param1.z),0.0f,1.0f);
		ImGui::SliderFloat("Shadow Offset Y",&(m_SDFData.param1.w),0.0f,1.0f);

		ImGui::Separator();

		ImGui::Text("%.3f ms/frame (%.1f FPS)",1000.0f/ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void SDFFontDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// model
	m_Model = DVKModel::LoadFromFile(
		"Assets/models/plane_z.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0
		}
	);
	m_Model->rootNode->localMatrix.RotateY(180.0f);
	m_Model->rootNode->localMatrix.SetScale(Vector3(3.15f,1.0f,1.0f),1.0f);

	// shader
	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/70_SDFFont/texture.vert.spv",
		"Assets/Shaders/70_SDFFont/texture.frag.spv"
	);

	// texture
	m_Texture = DVKTexture::Create2D(
		"Assets/textures/sdf.png",
		m_VulkanDevice,
		cmdBuffer
	);

	// material
	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_Material->pipelineInfo.blendAttachmentStates[0].blendEnable = VK_TRUE;
	m_Material->pipelineInfo.blendAttachmentStates[0].colorBlendOp = VK_BLEND_OP_ADD;
	m_Material->pipelineInfo.blendAttachmentStates[0].srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	m_Material->pipelineInfo.blendAttachmentStates[0].dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	m_Material->pipelineInfo.blendAttachmentStates[0].alphaBlendOp = VK_BLEND_OP_ADD;
	m_Material->pipelineInfo.blendAttachmentStates[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	m_Material->pipelineInfo.blendAttachmentStates[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	m_Material->PreparePipeline();
	m_Material->SetTexture("textureMap",m_Texture);

	delete cmdBuffer;
}

void SDFFontDemo::DestroyAssets()
{
	delete m_Shader;
	delete m_Texture;
	delete m_Material;
	delete m_Model;
}

void SDFFontDemo::SetupCommandBuffers(int32 backBufferIndex)
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
	for(int32 j = 0; j<m_Model->meshes.size(); ++j)
	{
		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,j);
		m_Model->meshes[j]->BindDrawCmd(commandBuffer);
	}

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void SDFFontDemo::InitParmas()
{
	m_SDFData.param0 = Vector4(0.5f,0.4f,0.25f,0.02f);
	m_SDFData.param1 = Vector4(0.1f,0.1f,0.01f,0.01f);
	m_SDFData.mainColor = Vector4(1.0f,1.0f,1.0f,1.0f);
	m_SDFData.outlineColor = Vector4(1.0f,0.0f,0.0f,1.0f);
	m_SDFData.glowColor = Vector4(0.0f,1.0f,0.0f,1.0f);
	m_SDFData.shadowColor = Vector4(0.0f,0.0f,0.0f,1.0f);

	m_ViewCamera.SetPosition(0,0,-2.5f);
	m_ViewCamera.Perspective(PI/4,GetWidth(),GetHeight(),0.10f,3000.0f);
}

void SDFFontDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void SDFFontDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}