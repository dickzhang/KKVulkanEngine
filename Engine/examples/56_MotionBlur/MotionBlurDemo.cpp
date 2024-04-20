#include "MotionBlurDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<MotionBlurDemo>(1400,900,"MotionBlurDemo",cmdLine);
}

MotionBlurDemo::MotionBlurDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

MotionBlurDemo::~MotionBlurDemo()
{

}

bool MotionBlurDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	CreateSourceRT();
	LoadAssets();

	m_Ready = true;
	return true;
}

void MotionBlurDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void MotionBlurDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void MotionBlurDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	auto bounds = m_Model->meshes[5]->bounding;
	Vector3 center = (bounds.max-bounds.min)*0.5f+bounds.min;
	m_Model->meshes[5]->linkNode->localMatrix.RotateZ(360*delta*m_Speed,true,&center);

	SetupCommandBuffers(bufferIndex);
	ModuleBase::Present(bufferIndex);

	for(int32 i = 0; i<MESH_SIZE; ++i)
	{
		m_PreviousMVP[i].model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_PreviousMVP[i].view = m_ViewCamera.GetView();
		m_PreviousMVP[i].proj = m_ViewCamera.GetProjection();
	}

#if PLATFORM_WINDOWS
	//Sleep(14);//»áµ¼ÖÂÍÏÓ°
#endif
}

bool MotionBlurDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("MotionBlurDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::SliderFloat("Speed",&m_Speed,0.1f,10.0f);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void MotionBlurDemo::CreateSourceRT()
{
	m_TexSourceColor = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);
	m_TexSourceColor->UpdateSampler(VK_FILTER_LINEAR,VK_FILTER_LINEAR,VK_SAMPLER_MIPMAP_MODE_LINEAR,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	m_TexSourceVelocity = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R16G16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);
	m_TexSourceVelocity->UpdateSampler(VK_FILTER_LINEAR,VK_FILTER_LINEAR,VK_SAMPLER_MIPMAP_MODE_LINEAR,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	m_TexSourceDepth = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(m_DepthFormat,false),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	DVKTexture* rtColors[2];
	rtColors[0] = m_TexSourceColor;
	rtColors[1] = m_TexSourceVelocity;

	DVKRenderPassInfo rttInfo(
		2,rtColors,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
		m_TexSourceDepth,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
	);
	m_RTSource = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
}

void MotionBlurDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// fullscreen
	m_Quad = DVKDefaultRes::fullQuad;

	// scene model
	m_Model = DVKModel::LoadFromFile(
		"Assets/models/Papercraft Windmills/Papercraft Windmills.obj",
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
		"Assets/Shaders/59_MotionBlur/obj.vert.spv",
		"Assets/Shaders/59_MotionBlur/obj.frag.spv"
	);

	const char* textures[MESH_SIZE] = {
		"Assets/models/Papercraft Windmills/terrain.jpg",
		"Assets/models/Papercraft Windmills/rocks.jpg",
		"Assets/models/Papercraft Windmills/stolbiki.jpg",
		"Assets/models/Papercraft Windmills/animals.jpg",
		"Assets/models/Papercraft Windmills/mill1day.jpg",
		"Assets/models/Papercraft Windmills/wingl1day.png",
		"Assets/models/Papercraft Windmills/trees.jpg",
		"Assets/models/Papercraft Windmills/telega.jpg",
		"Assets/models/Papercraft Windmills/sky_clear.jpg",
		"Assets/models/Papercraft Windmills/wingl2day.png",
		"Assets/models/Papercraft Windmills/mill2day.jpg"
	};

	for(int32 i = 0; i<MESH_SIZE; ++i)
	{
		m_Textures[i] = DVKTexture::Create2D(
			textures[i],
			m_VulkanDevice,
			cmdBuffer
		);

		m_Materials[i] = DVKMaterial::Create(
			m_VulkanDevice,
			m_RTSource->GetRenderPass(),
			m_PipelineCache,
			m_Shader
		);
		m_Materials[i]->pipelineInfo.colorAttachmentCount = 2;
		m_Materials[i]->PreparePipeline();
		m_Materials[i]->SetTexture("diffuseMap",m_Textures[i]);
	}

	// final
	m_CombineShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/59_MotionBlur/combine.vert.spv",
		"Assets/Shaders/59_MotionBlur/combine.frag.spv"
	);

	m_CombineMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_CombineShader
	);
	m_CombineMaterial->PreparePipeline();
	m_CombineMaterial->SetTexture("originTexture",m_TexSourceColor);
	m_CombineMaterial->SetTexture("velocityTexture",m_TexSourceVelocity);

	delete cmdBuffer;
}

void MotionBlurDemo::DestroyAssets()
{
	delete m_Model;
	delete m_Shader;

	for(int32 i = 0; i<MESH_SIZE; ++i)
	{
		delete m_Textures[i];
		delete m_Materials[i];
	}

	// source
	{
		delete m_TexSourceColor;
		delete m_TexSourceVelocity;
		delete m_TexSourceDepth;
		delete m_RTSource;
	}

	// final
	{
		delete m_CombineShader;
		delete m_CombineMaterial;
	}
}

void MotionBlurDemo::VelocityPass(VkCommandBuffer commandBuffer)
{
	m_RTSource->BeginRenderPass(commandBuffer);

	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Materials[i]->GetPipeline());

		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Materials[i]->BeginFrame();
		m_Materials[i]->BeginObject();
		m_Materials[i]->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Materials[i]->SetLocalUniform("uboPrevMVP",&(m_PreviousMVP[i]),sizeof(ModelViewProjectionBlock));
		m_Materials[i]->EndObject();
		m_Materials[i]->EndFrame();

		m_Materials[i]->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);
	}

	m_RTSource->EndRenderPass(commandBuffer);
}

void MotionBlurDemo::RenderFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex)
{
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

	// combine pass
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

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugParam.x = m_Enable ? 1.0f : 0.0f;

		m_CombineMaterial->BeginFrame();
		m_CombineMaterial->BeginObject();
		m_CombineMaterial->SetLocalUniform("paramData",&m_DebugParam,sizeof(Vector4));
		m_CombineMaterial->EndObject();
		m_CombineMaterial->EndFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_CombineMaterial->GetPipeline());
		m_CombineMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);
	}

	// ui pass
	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
}

void MotionBlurDemo::SetupCommandBuffers(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	VelocityPass(commandBuffer);
	RenderFinal(commandBuffer,backBufferIndex);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void MotionBlurDemo::InitParmas()
{
	m_ViewCamera.SetPosition(-425.0f,445.0f,-845.0f);
	m_ViewCamera.SetOrientation(Vector3(0.663033128f,-0.179494619f,0.726750910f));
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,5000.0f);
	m_ViewCamera.speedFactor = 2.0f;
	m_ViewCamera.smooth = 0.5f;

	for(int32 i = 0; i<MESH_SIZE; ++i)
	{
		m_PreviousMVP[i].model.SetIdentity();
		m_PreviousMVP[i].view = m_ViewCamera.GetView();
		m_PreviousMVP[i].proj = m_ViewCamera.GetProjection();
	}
}

void MotionBlurDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void MotionBlurDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}