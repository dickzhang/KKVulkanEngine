#include "DepthPeelingDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<DepthPeelingDemo>(1400,900,"DepthPeelingDemo",cmdLine);
}

DepthPeelingDemo::DepthPeelingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

DepthPeelingDemo::~DepthPeelingDemo()
{

}

bool DepthPeelingDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	CreateRendertarget();
	LoadAssets();

	m_Ready = true;
	return true;
}

void DepthPeelingDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void DepthPeelingDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void DepthPeelingDemo::Draw(float time,float delta)
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

bool DepthPeelingDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("DepthPeelingDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		float bias = m_PeelParam.y*100000;
		ImGui::SliderFloat("Bias",&bias,0,100);
		m_PeelParam.y = bias/100000;

		int32 layer = m_PeelParam.z;
		ImGui::SliderInt("Debug",&layer,0,5);
		m_PeelParam.z = layer;

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void DepthPeelingDemo::CreateRendertarget()
{
	// Opaque
	m_TexSourceColor = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	m_TexSourceDepth = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(m_DepthFormat,false),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	{
		DVKRenderPassInfo rttInfo(
			m_TexSourceColor,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
			m_TexSourceDepth,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
		);
		m_RTSource = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}

	// temp depth
	m_TexDepth0 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(m_DepthFormat,false),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	m_TexDepth1 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		PixelFormatToVkFormat(m_DepthFormat,false),
		VK_IMAGE_ASPECT_DEPTH_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	// peel0
	m_TexPeel0 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	{
		// use m_TexSourceDepth and write to depth0
		DVKRenderPassInfo rttInfo(
			m_TexPeel0,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
			m_TexDepth0,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
		);
		m_RTPeel0 = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}

	// peel1
	m_TexPeel1 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	{
		// use depth0 and write to depth1
		DVKRenderPassInfo rttInfo(
			m_TexPeel1,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
			m_TexDepth1,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
		);
		m_RTPeel1 = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}

	// peel2
	m_TexPeel2 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	{
		// use depth1 and write to depth0
		DVKRenderPassInfo rttInfo(
			m_TexPeel2,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
			m_TexDepth0,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
		);
		m_RTPeel2 = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}

	// peel3
	m_TexPeel3 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	{
		// use depth0 and write to depth1
		DVKRenderPassInfo rttInfo(
			m_TexPeel3,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
			m_TexDepth1,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
		);
		m_RTPeel3 = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}

	// peel4
	m_TexPeel4 = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	{
		// use depth1 and write to depth0
		DVKRenderPassInfo rttInfo(
			m_TexPeel4,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
			m_TexDepth0,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
		);
		m_RTPeel4 = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}
}

void DepthPeelingDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// fullscreen
	m_Quad = DVKDefaultRes::fullQuad;

	// scene model
	m_Model = DVKModel::LoadFromFile(
		"Assets/models/plane_z.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0,
			VertexAttribute::VA_Normal
		}
	);
	m_Model->rootNode->localMatrix.AppendScale(Vector3(5,5,5));
	m_Model->rootNode->localMatrix.AppendRotation(180.0f,Vector3::UpVector);
	m_Model->rootNode->localMatrix.AppendTranslation(Vector3(0,0,2.5f));

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/60_DepthPeeling/obj.vert.spv",
		"Assets/Shaders/60_DepthPeeling/obj.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTSource->GetRenderPass(),
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();

	// susan
	m_SuzanneModel = DVKModel::LoadFromFile(
		"Assets/models/dragon.obj",
		m_VulkanDevice,
		cmdBuffer,
		{ VertexAttribute::VA_Position,VertexAttribute::VA_UV0,VertexAttribute::VA_Normal }
	);
	m_SuzanneModel->rootNode->localMatrix.AppendRotation(180.0f,Vector3::UpVector);

	m_PeelShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/60_DepthPeeling/peel.vert.spv",
		"Assets/Shaders/60_DepthPeeling/peel.frag.spv"
	);

	m_PeelMaterials[0] = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTPeel0->GetRenderPass(),
		m_PipelineCache,
		m_PeelShader
	);
	m_PeelMaterials[0]->PreparePipeline();
	m_PeelMaterials[0]->SetTexture("peelMap",m_TexSourceDepth);

	m_PeelMaterials[1] = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTPeel1->GetRenderPass(),
		m_PipelineCache,
		m_PeelShader
	);
	m_PeelMaterials[1]->PreparePipeline();
	m_PeelMaterials[1]->SetTexture("peelMap",m_TexDepth0);

	m_PeelMaterials[2] = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTPeel2->GetRenderPass(),
		m_PipelineCache,
		m_PeelShader
	);
	m_PeelMaterials[2]->PreparePipeline();
	m_PeelMaterials[2]->SetTexture("peelMap",m_TexDepth1);

	m_PeelMaterials[3] = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTPeel3->GetRenderPass(),
		m_PipelineCache,
		m_PeelShader
	);
	m_PeelMaterials[3]->PreparePipeline();
	m_PeelMaterials[3]->SetTexture("peelMap",m_TexDepth0);

	m_PeelMaterials[4] = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTPeel4->GetRenderPass(),
		m_PipelineCache,
		m_PeelShader
	);
	m_PeelMaterials[4]->PreparePipeline();
	m_PeelMaterials[4]->SetTexture("peelMap",m_TexDepth1);

	// final
	m_CombineShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/60_DepthPeeling/combine.vert.spv",
		"Assets/Shaders/60_DepthPeeling/combine.frag.spv"
	);

	m_CombineMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_CombineShader
	);
	m_CombineMaterial->PreparePipeline();
	m_CombineMaterial->SetTexture("originTexture",m_TexSourceColor);
	m_CombineMaterial->SetTexture("peel0Texture",m_TexPeel0);
	m_CombineMaterial->SetTexture("peel1Texture",m_TexPeel1);
	m_CombineMaterial->SetTexture("peel2Texture",m_TexPeel2);
	m_CombineMaterial->SetTexture("peel3Texture",m_TexPeel3);
	m_CombineMaterial->SetTexture("peel4Texture",m_TexPeel4);

	delete cmdBuffer;
}

void DepthPeelingDemo::DestroyAssets()
{
	{
		delete m_Model;
		delete m_Shader;
		delete m_Material;
	}

	// source
	{
		delete m_TexSourceColor;
		delete m_TexSourceDepth;
		delete m_RTSource;
	}

	// m_SuzanneModel
	{
		delete m_SuzanneModel;
		delete m_PeelShader;
		delete m_PeelMaterials[0];
		delete m_PeelMaterials[1];
		delete m_PeelMaterials[2];
		delete m_PeelMaterials[3];
		delete m_PeelMaterials[4];
	}

	// final
	{
		delete m_CombineShader;
		delete m_CombineMaterial;
	}

	// peel
	{
		delete m_TexDepth0;
		delete m_TexDepth1;

		delete m_TexPeel0;
		delete m_RTPeel0;

		delete m_TexPeel1;
		delete m_RTPeel1;

		delete m_TexPeel2;
		delete m_RTPeel2;

		delete m_TexPeel3;
		delete m_RTPeel3;

		delete m_TexPeel4;
		delete m_RTPeel4;
	}
}

void DepthPeelingDemo::OpaquePass(VkCommandBuffer commandBuffer)
{
	m_RTSource->BeginRenderPass(commandBuffer);

	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());

		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Material->BeginFrame();
		m_Material->BeginObject();
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->EndObject();
		m_Material->EndFrame();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);
	}

	m_RTSource->EndRenderPass(commandBuffer);
}

void DepthPeelingDemo::RenderFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex)
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

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_CombineMaterial->GetPipeline());

		m_CombineMaterial->BeginFrame();
		m_CombineMaterial->BeginObject();
		m_CombineMaterial->SetLocalUniform("uboParam",&m_PeelParam,sizeof(Vector4));
		m_CombineMaterial->EndObject();
		m_CombineMaterial->EndFrame();

		m_CombineMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);
	}

	// ui pass
	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
}

void DepthPeelingDemo::PeelPass(VkCommandBuffer commandBuffer,DVKMaterial* material,DVKRenderTarget* renderTarget,int32 layer)
{
	renderTarget->BeginRenderPass(commandBuffer);

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,material->GetPipeline());

	m_MVPParam.model = m_SuzanneModel->meshes[0]->linkNode->GetGlobalMatrix();
	m_MVPParam.view = m_ViewCamera.GetView();
	m_MVPParam.proj = m_ViewCamera.GetProjection();

	m_PeelParam.x = layer;

	material->BeginFrame();
	material->BeginObject();
	material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
	material->SetLocalUniform("uboParam",&m_PeelParam,sizeof(Vector4));
	material->EndObject();
	material->EndFrame();

	material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_SuzanneModel->meshes[0]->BindDrawCmd(commandBuffer);

	renderTarget->EndRenderPass(commandBuffer);
}

void DepthPeelingDemo::SetupCommandBuffers(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	OpaquePass(commandBuffer);
	PeelPass(commandBuffer,m_PeelMaterials[0],m_RTPeel0,0);
	PeelPass(commandBuffer,m_PeelMaterials[1],m_RTPeel1,1);
	PeelPass(commandBuffer,m_PeelMaterials[2],m_RTPeel2,2);
	PeelPass(commandBuffer,m_PeelMaterials[3],m_RTPeel3,3);
	PeelPass(commandBuffer,m_PeelMaterials[4],m_RTPeel4,4);
	RenderFinal(commandBuffer,backBufferIndex);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void DepthPeelingDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,0,-5);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,1000.0f);

	m_PeelParam.x = 0;
	m_PeelParam.y = 0.00005;
	m_PeelParam.z = 0;
}

void DepthPeelingDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void DepthPeelingDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}