#include "GodRayDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<GodRayDemo>(1400,900,"GodRayDemo",cmdLine);
}

GodRayDemo::GodRayDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

GodRayDemo::~GodRayDemo()
{

}

bool GodRayDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	CreateSourceRT();
	CreateSunRT();
	LoadModelAssets();
	InitParmas();

	m_Ready = true;
	return true;
}

void GodRayDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void GodRayDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void GodRayDemo::Draw(float time,float delta)
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

bool GodRayDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("GodRayDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::SliderFloat("Density",&m_CombineParam.param.x,0.0f,5.0f);
		ImGui::SliderFloat("Decay",&m_CombineParam.param.y,0.0f,1.0f);
		ImGui::SliderFloat("Weight",&m_CombineParam.param.z,0.0f,1.0f);
		ImGui::SliderFloat("Exposure",&m_CombineParam.param.w,0.0f,10.0f);

		ImGui::Separator();

		ImGui::ColorEdit3("Color",(float*)&m_CombineParam.color);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void GodRayDemo::CreateSunRT()
{
	m_TexLight = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R8G8B8A8_UNORM,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth,
		m_FrameHeight,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	DVKRenderPassInfo rttInfo(
		m_TexLight,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
		m_TexSourceDepth,VK_ATTACHMENT_LOAD_OP_DONT_CARE,VK_ATTACHMENT_STORE_OP_DONT_CARE
	);
	m_RTLight = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
}

void GodRayDemo::CreateSourceRT()
{
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

	DVKRenderPassInfo rttInfo(
		m_TexSourceColor,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,
		m_TexSourceDepth,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE
	);
	m_RTSource = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
}

void GodRayDemo::LoadModelAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_SphereModel = DVKModel::LoadFromFile(
		"Assets/models/sphere1.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0
		}
	);

	// object
	m_Model = DVKModel::LoadFromFile(
		"Assets/models/halloween-pumpkin/model.fbx",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0,
			VertexAttribute::VA_Normal,
			VertexAttribute::VA_Tangent
		}
	);
	m_Model->rootNode->localMatrix.AppendRotation(180,Vector3::UpVector);

	m_TexAlbedo = DVKTexture::Create2D(
		"Assets/models/halloween-pumpkin/BaseColor.jpg",
		m_VulkanDevice,
		cmdBuffer
	);

	m_TexNormal = DVKTexture::Create2D(
		"Assets/models/halloween-pumpkin/Normal.jpg",
		m_VulkanDevice,
		cmdBuffer
	);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/57_GodRay/obj.vert.spv",
		"Assets/Shaders/57_GodRay/obj.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTSource->GetRenderPass(),
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();
	m_Material->SetTexture("texAlbedo",m_TexAlbedo);
	m_Material->SetTexture("texNormal",m_TexNormal);

	// emissive
	m_EmissiveShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/57_GodRay/emissive.vert.spv",
		"Assets/Shaders/57_GodRay/emissive.frag.spv"
	);

	m_EmissiveMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTLight->GetRenderPass(),
		m_PipelineCache,
		m_EmissiveShader
	);
	m_EmissiveMaterial->PreparePipeline();

	// combine
	m_CombineShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/57_GodRay/combine.vert.spv",
		"Assets/Shaders/57_GodRay/combine.frag.spv"
	);

	m_CombineMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_CombineShader
	);
	m_CombineMaterial->PreparePipeline();
	m_CombineMaterial->SetTexture("originTexture",m_TexSourceColor);
	m_CombineMaterial->SetTexture("lightTexture",m_TexLight);

	delete cmdBuffer;
}

void GodRayDemo::DestroyAssets()
{
	delete m_Model;
	delete m_SphereModel;

	delete m_Shader;
	delete m_Material;

	delete m_TexAlbedo;
	delete m_TexNormal;

	delete m_TexSourceColor;
	delete m_TexSourceDepth;
	delete m_RTSource;

	delete m_CombineShader;
	delete m_CombineMaterial;

	delete m_EmissiveShader;
	delete m_EmissiveMaterial;

	delete m_RTLight;
	delete m_TexLight;
}

void GodRayDemo::DrawScene(VkCommandBuffer commandBuffer)
{
	m_RTSource->BeginRenderPass(commandBuffer);

	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());

		m_Material->BeginFrame();

		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Material->BeginObject();
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->EndObject();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);

		m_Material->EndFrame();
	}

	m_RTSource->EndRenderPass(commandBuffer);
}

void GodRayDemo::DrawFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex)
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

	VkViewport viewport = { };
	viewport.x = 0;
	viewport.y = m_FrameHeight;
	viewport.width = m_FrameWidth;
	viewport.height = -m_FrameHeight;    // flip y axis
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.extent.width = m_FrameWidth;
	scissor.extent.height = m_FrameHeight;
	scissor.offset.x = 0;
	scissor.offset.y = 0;

	vkCmdSetViewport(commandBuffer,0,1,&viewport);
	vkCmdSetScissor(commandBuffer,0,1,&scissor);

	m_CombineMaterial->BeginFrame();
	m_CombineMaterial->BeginObject();
	m_CombineMaterial->SetLocalUniform("paramData",&m_CombineParam,sizeof(ParamBlock));
	m_CombineMaterial->EndObject();
	m_CombineMaterial->EndFrame();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_CombineMaterial->GetPipeline());
	m_CombineMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	DVKDefaultRes::fullQuad->meshes[0]->BindDrawCmd(commandBuffer);

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
}

void GodRayDemo::DrawLight(VkCommandBuffer commandBuffer)
{
	m_RTLight->BeginRenderPass(commandBuffer);

	m_MVPParam.model.SetIdentity();
	m_MVPParam.model.AppendScale(Vector3(0.75f,0.75f,0.75f));
	m_MVPParam.view = m_ViewCamera.GetView();
	m_MVPParam.proj = m_ViewCamera.GetProjection();

	m_EmissiveMaterial->BeginFrame();
	m_EmissiveMaterial->BeginObject();
	m_EmissiveMaterial->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
	m_EmissiveMaterial->EndObject();
	m_EmissiveMaterial->EndFrame();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_EmissiveMaterial->GetPipeline());
	m_EmissiveMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_SphereModel->meshes[0]->BindDrawCmd(commandBuffer);

	m_RTLight->EndRenderPass(commandBuffer);
}

void GodRayDemo::SetupCommandBuffers(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	DrawScene(commandBuffer);
	DrawLight(commandBuffer);
	DrawFinal(commandBuffer,backBufferIndex);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void GodRayDemo::InitParmas()
{
	DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;

	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y,boundCenter.z-5.0f);
	m_ViewCamera.LookAt(boundCenter);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,100.0f);

	m_CombineParam.param.x = 0.65f;
	m_CombineParam.param.y = 0.85f;
	m_CombineParam.param.z = 0.75f;
	m_CombineParam.param.w = 1.25f;

	m_CombineParam.color.x = 0.785f;
	m_CombineParam.color.y = 1.0f;
	m_CombineParam.color.z = 0.0f;
	m_CombineParam.color.w = 1.0f;
}

void GodRayDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void GodRayDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}