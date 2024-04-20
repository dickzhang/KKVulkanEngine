#include "HDRPipelineDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<HDRPipelineDemo>(1400,900,"HDRPipelineDemo",cmdLine);
}

HDRPipelineDemo::HDRPipelineDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

HDRPipelineDemo::~HDRPipelineDemo()
{

}

bool HDRPipelineDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	CreateSourceRT();
	CreateBrightRT();
	CreateBlurRT();
	CreateLuminanceRT();
	LoadAssets();

	m_Ready = true;
	return true;
}

void HDRPipelineDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void HDRPipelineDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void HDRPipelineDemo::Draw(float time,float delta)
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

bool HDRPipelineDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("HDRPipelineDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Checkbox("Debug",&m_Debug);

		ImGui::SliderFloat("Intensity",&m_ParamData.intensity.x,1.0f,10.0f);
		ImGui::SliderFloat("Exposure",&m_ParamData.intensity.y,0.0f,5.0f);
		ImGui::SliderFloat("bias",&m_ParamData.intensity.w,0.0f,5.0f);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void HDRPipelineDemo::CreateLuminanceRT()
{
	// down sample
	m_TexLuminances[6] = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_TexSourceColor->width/4,
		m_TexSourceColor->height/4,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	// Luminance
	for(int32 i = 0; i<6; ++i)
	{
		m_TexLuminances[i] = DVKTexture::CreateRenderTarget(
			m_VulkanDevice,
			VK_FORMAT_R16_SFLOAT,
			VK_IMAGE_ASPECT_COLOR_BIT,
			MMath::Pow(3,i),
			MMath::Pow(3,i),
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
		);
	}

	for(int32 i = 0; i<6; ++i)
	{
		m_TexLuminances[i]->UpdateSampler(
			VK_FILTER_NEAREST,
			VK_FILTER_NEAREST,
			VK_SAMPLER_MIPMAP_MODE_NEAREST,
			VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
		);
	}

	// render target pass
	for(int32 i = 0; i<7; ++i)
	{
		DVKRenderPassInfo rttInfo(
			m_TexLuminances[i],VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
		);
		m_RTLuminances[i] = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	}

	// down sample shader
	m_LuminanceDowmSampleShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/luminanceDownsample.vert.spv",
		"Assets/Shaders/52_HDRPipeline/luminanceDownsample.frag.spv"
	);

	m_LuminanceMaterials[6] = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTLuminances[6]->GetRenderPass(),
		m_PipelineCache,
		m_LuminanceDowmSampleShader
	);
	m_LuminanceMaterials[6]->PreparePipeline();
	m_LuminanceMaterials[6]->SetTexture("originTexture",m_TexSourceColor);

	// luminance shader
	m_LuminanceShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/luminance.vert.spv",
		"Assets/Shaders/52_HDRPipeline/luminance.frag.spv"
	);

	for(int32 i = 0; i<6; ++i)
	{
		m_LuminanceMaterials[i] = DVKMaterial::Create(
			m_VulkanDevice,
			m_RTLuminances[i]->GetRenderPass(),
			m_PipelineCache,
			m_LuminanceShader
		);
		m_LuminanceMaterials[i]->PreparePipeline();
		m_LuminanceMaterials[i]->SetTexture("originTexture",m_TexLuminances[i+1]);
	}
}

void HDRPipelineDemo::CreateBlurRT()
{
	// blurH
	m_TexBlurH = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		m_TexBright->format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_TexBright->width,
		m_TexBright->height,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	DVKRenderPassInfo rttInfoH(
		m_TexBlurH,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
	);
	m_RTBlurH = DVKRenderTarget::Create(m_VulkanDevice,rttInfoH);

	m_BlurHShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/blurH.vert.spv",
		"Assets/Shaders/52_HDRPipeline/blurH.frag.spv"
	);

	m_BlurHMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTBlurH->GetRenderPass(),
		m_PipelineCache,
		m_BlurHShader
	);
	m_BlurHMaterial->PreparePipeline();
	m_BlurHMaterial->SetTexture("originTexture",m_TexBright);

	// blurV
	m_TexBlurV = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		m_TexBlurH->format,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_TexBlurH->width,
		m_TexBlurH->height,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	DVKRenderPassInfo rttInfoV(
		m_TexBlurV,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
	);
	m_RTBlurV = DVKRenderTarget::Create(m_VulkanDevice,rttInfoV);

	m_BlurVShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/blurV.vert.spv",
		"Assets/Shaders/52_HDRPipeline/blurV.frag.spv"
	);

	m_BlurVMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTBlurV->GetRenderPass(),
		m_PipelineCache,
		m_BlurVShader
	);
	m_BlurVMaterial->PreparePipeline();
	m_BlurVMaterial->SetTexture("originTexture",m_TexBlurH);
}

void HDRPipelineDemo::CreateBrightRT()
{
	m_TexBright = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		m_FrameWidth/4.0f,
		m_FrameHeight/4.0f,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT
	);

	DVKRenderPassInfo rttInfo(
		m_TexBright,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
	);
	m_RTBright = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);

	m_BrightShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/bright.vert.spv",
		"Assets/Shaders/52_HDRPipeline/bright.frag.spv"
	);

	m_BrightMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RTBright->GetRenderPass(),
		m_PipelineCache,
		m_BrightShader
	);
	m_BrightMaterial->PreparePipeline();
	m_BrightMaterial->SetTexture("originTexture",m_TexSourceColor);
}

void HDRPipelineDemo::CreateSourceRT()
{
	m_TexSourceColor = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R16G16B16A16_SFLOAT,
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

void HDRPipelineDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// fullscreen
	m_Quad = DVKDefaultRes::fullQuad;

	// scene model
	m_SceneModel = DVKModel::LoadFromFile(
		"Assets/models/Portal/Portal_FInal.fbx",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0,
			VertexAttribute::VA_Normal
		}
	);

	m_SceneShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/obj.vert.spv",
		"Assets/Shaders/52_HDRPipeline/obj.frag.spv"
	);

	const char* textures[2] = {
		"Assets/models/Portal/Portal_Main.jpg",
		"Assets/models/Portal/Portal1.png"
	};

	for(int32 i = 0; i<2; ++i)
	{
		m_SceneTextures[i] = DVKTexture::Create2D(
			textures[i],
			m_VulkanDevice,
			cmdBuffer
		);

		m_SceneMaterials[i] = DVKMaterial::Create(
			m_VulkanDevice,
			m_RTSource->GetRenderPass(),
			m_PipelineCache,
			m_SceneShader
		);
		m_SceneMaterials[i]->PreparePipeline();
		m_SceneMaterials[i]->SetTexture("diffuseMap",m_SceneTextures[i]);
	}

	// final
	m_FinalShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/combine.vert.spv",
		"Assets/Shaders/52_HDRPipeline/combine.frag.spv"
	);

	m_FinalMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_FinalShader
	);
	m_FinalMaterial->PreparePipeline();
	m_FinalMaterial->SetTexture("originTexture",m_TexSourceColor);
	m_FinalMaterial->SetTexture("bloomTexture",m_TexBlurV);
	m_FinalMaterial->SetTexture("luminanceTexture",m_TexLuminances[0]);

	// for debug
	m_DebugShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/52_HDRPipeline/debug.vert.spv",
		"Assets/Shaders/52_HDRPipeline/debug.frag.spv"
	);

	m_DebugBright = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugBright->PreparePipeline();
	m_DebugBright->SetTexture("originTexture",m_TexBright);

	m_DebugBlurH = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugBlurH->PreparePipeline();
	m_DebugBlurH->SetTexture("originTexture",m_TexBlurH);

	m_DebugBlurV = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugBlurV->PreparePipeline();
	m_DebugBlurV->SetTexture("originTexture",m_TexBlurV);

	m_DebugLumDownsample = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLumDownsample->PreparePipeline();
	m_DebugLumDownsample->SetTexture("originTexture",m_TexLuminances[6]);

	m_DebugLum1x1 = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLum1x1->PreparePipeline();
	m_DebugLum1x1->SetTexture("originTexture",m_TexLuminances[0]);

	m_DebugLum3x3 = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLum3x3->PreparePipeline();
	m_DebugLum3x3->SetTexture("originTexture",m_TexLuminances[1]);

	m_DebugLum9x9 = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLum9x9->PreparePipeline();
	m_DebugLum9x9->SetTexture("originTexture",m_TexLuminances[2]);

	m_DebugLum27x27 = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLum27x27->PreparePipeline();
	m_DebugLum27x27->SetTexture("originTexture",m_TexLuminances[3]);

	m_DebugLum81x81 = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLum81x81->PreparePipeline();
	m_DebugLum81x81->SetTexture("originTexture",m_TexLuminances[4]);

	m_DebugLum243x243 = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_DebugShader
	);
	m_DebugLum243x243->PreparePipeline();
	m_DebugLum243x243->SetTexture("originTexture",m_TexLuminances[5]);

	delete cmdBuffer;
}

void HDRPipelineDemo::DestroyAssets()
{
	delete m_SceneModel;
	delete m_SceneShader;

	delete m_DebugShader;
	delete m_DebugBright;
	delete m_DebugBlurH;
	delete m_DebugBlurV;
	delete m_DebugLumDownsample;
	delete m_DebugLum1x1;
	delete m_DebugLum3x3;
	delete m_DebugLum9x9;
	delete m_DebugLum27x27;
	delete m_DebugLum81x81;
	delete m_DebugLum243x243;

	for(int32 i = 0; i<2; ++i)
	{
		delete m_SceneTextures[i];
		delete m_SceneMaterials[i];
	}

	// source
	{
		delete m_TexSourceColor;
		delete m_TexSourceDepth;
		delete m_RTSource;
	}

	// bright
	{
		delete m_TexBright;
		delete m_RTBright;
		delete m_BrightShader;
		delete m_BrightMaterial;
	}

	// blur h
	{
		delete m_TexBlurH;
		delete m_RTBlurH;
		delete m_BlurHShader;
		delete m_BlurHMaterial;
	}

	// blur v
	{
		delete m_TexBlurV;
		delete m_RTBlurV;
		delete m_BlurVShader;
		delete m_BlurVMaterial;
	}

	// luminance
	{
		for(int32 i = 0; i<7; ++i)
		{
			delete m_TexLuminances[i];
			delete m_RTLuminances[i];
			delete m_LuminanceMaterials[i];
		}
		delete m_LuminanceDowmSampleShader;
		delete m_LuminanceShader;
	}

	// final
	{
		delete m_FinalShader;
		delete m_FinalMaterial;
	}
}

void HDRPipelineDemo::RenderScene(VkCommandBuffer commandBuffer)
{
	DVKMaterial* materials[4] = {
		m_SceneMaterials[1],
		m_SceneMaterials[0],
		m_SceneMaterials[1],
		m_SceneMaterials[1]
	};

	float params[4] = {
		m_ParamData.intensity.x,
		1,
		m_ParamData.intensity.x,
		m_ParamData.intensity.x
	};

	for(int32 i = 0; i<m_SceneModel->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,materials[i]->GetPipeline());

		materials[i]->BeginFrame();

		m_MVPParam.model = m_SceneModel->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_ParamData.intensity.x = params[i];

		materials[i]->BeginObject();
		materials[i]->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		materials[i]->SetLocalUniform("paramData",&m_ParamData,sizeof(ParamBlock));
		materials[i]->EndObject();

		materials[i]->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_SceneModel->meshes[i]->BindDrawCmd(commandBuffer);

		materials[i]->EndFrame();
	}

	// restore
	m_ParamData.intensity.x = params[0];
}

void HDRPipelineDemo::SourcePass(VkCommandBuffer commandBuffer)
{
	m_RTSource->BeginRenderPass(commandBuffer);
	RenderScene(commandBuffer);
	m_RTSource->EndRenderPass(commandBuffer);
}

void HDRPipelineDemo::BrightPass(VkCommandBuffer commandBuffer)
{
	m_RTBright->BeginRenderPass(commandBuffer);
	m_BrightMaterial->BeginFrame();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_BrightMaterial->GetPipeline());
	m_BrightMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

	m_BrightMaterial->EndFrame();
	m_RTBright->EndRenderPass(commandBuffer);
}

void HDRPipelineDemo::BlurHPass(VkCommandBuffer commandBuffer)
{
	m_RTBlurH->BeginRenderPass(commandBuffer);
	m_BlurHMaterial->BeginFrame();

	m_BlurHMaterial->BeginObject();
	m_BlurHMaterial->SetLocalUniform("paramData",&m_ParamData,sizeof(ParamBlock));
	m_BlurHMaterial->EndObject();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_BlurHMaterial->GetPipeline());
	m_BlurHMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

	m_BlurHMaterial->EndFrame();
	m_RTBlurH->EndRenderPass(commandBuffer);
}

void HDRPipelineDemo::LuminancePass(VkCommandBuffer commandBuffer)
{
	// downsample first
	// Luminance one by one
	for(int32 i = 6; i>=0; --i)
	{
		m_RTLuminances[i]->BeginRenderPass(commandBuffer);
		m_LuminanceMaterials[i]->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_LuminanceMaterials[i]->GetPipeline());
		m_LuminanceMaterials[i]->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_LuminanceMaterials[i]->EndFrame();
		m_RTLuminances[i]->EndRenderPass(commandBuffer);
	}
}

void HDRPipelineDemo::BlurVPass(VkCommandBuffer commandBuffer)
{
	m_RTBlurV->BeginRenderPass(commandBuffer);
	m_BlurVMaterial->BeginFrame();

	m_BlurVMaterial->BeginObject();
	m_BlurVMaterial->SetLocalUniform("paramData",&m_ParamData,sizeof(ParamBlock));
	m_BlurVMaterial->EndObject();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_BlurVMaterial->GetPipeline());
	m_BlurVMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

	m_BlurVMaterial->EndFrame();
	m_RTBlurV->EndRenderPass(commandBuffer);
}

void HDRPipelineDemo::RenderPipeline(VkCommandBuffer commandBuffer)
{
	VkViewport viewport = { };
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	VkRect2D scissor = { };

	viewport.width = m_FrameWidth*0.25f;
	viewport.height = m_FrameHeight*0.25f*-1.0f;

	scissor.extent.width = m_FrameWidth*0.25f;
	scissor.extent.height = m_FrameHeight*0.25f;

	// bright
	{
		viewport.x = m_FrameWidth*0.25f;
		viewport.y = m_FrameHeight*0.25f;
		scissor.offset.x = m_FrameWidth*0.25f;
		scissor.offset.y = 0;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugBright->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugBright->GetPipeline());
		m_DebugBright->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugBright->EndFrame();
	}

	// blurH pass
	{
		viewport.x = m_FrameWidth*0.50f;
		viewport.y = m_FrameHeight*0.25f;
		scissor.offset.x = m_FrameWidth*0.50f;
		scissor.offset.y = 0;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugBlurH->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugBlurH->GetPipeline());
		m_DebugBlurH->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugBlurH->EndFrame();
	}

	// blurV pass
	{
		viewport.x = m_FrameWidth*0.75f;
		viewport.y = m_FrameHeight*0.25f;
		scissor.offset.x = m_FrameWidth*0.75f;
		scissor.offset.y = 0;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugBlurV->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugBlurV->GetPipeline());
		m_DebugBlurV->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugBlurV->EndFrame();
	}

	// down sample
	{
		viewport.x = 0;
		viewport.y = m_FrameHeight*0.50f;
		scissor.offset.x = 0;
		scissor.offset.y = m_FrameHeight*0.25f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLumDownsample->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLumDownsample->GetPipeline());
		m_DebugLumDownsample->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLumDownsample->EndFrame();
	}

	// 1x1
	{
		viewport.x = 0;
		viewport.y = m_FrameHeight*0.75f;
		scissor.offset.x = 0;
		scissor.offset.y = m_FrameHeight*0.50f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLum1x1->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLum1x1->GetPipeline());
		m_DebugLum1x1->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLum1x1->EndFrame();
	}

	// 3x3
	{
		viewport.x = 0;
		viewport.y = m_FrameHeight;
		scissor.offset.x = 0;
		scissor.offset.y = m_FrameHeight*0.75f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLum3x3->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLum3x3->GetPipeline());
		m_DebugLum3x3->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLum3x3->EndFrame();
	}

	// 9x9
	{
		viewport.x = m_FrameWidth*0.25f;
		viewport.y = m_FrameHeight;
		scissor.offset.x = m_FrameWidth*0.25f;
		scissor.offset.y = m_FrameHeight*0.75f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLum9x9->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLum9x9->GetPipeline());
		m_DebugLum9x9->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLum9x9->EndFrame();
	}

	// 27x27
	{
		viewport.x = m_FrameWidth*0.50f;
		viewport.y = m_FrameHeight;
		scissor.offset.x = m_FrameWidth*0.50f;
		scissor.offset.y = m_FrameHeight*0.75f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLum27x27->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLum27x27->GetPipeline());
		m_DebugLum27x27->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLum27x27->EndFrame();
	}

	// 81x81
	{
		viewport.x = m_FrameWidth*0.75f;
		viewport.y = m_FrameHeight;
		scissor.offset.x = m_FrameWidth*0.75f;
		scissor.offset.y = m_FrameHeight*0.75f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLum81x81->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLum81x81->GetPipeline());
		m_DebugLum81x81->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLum81x81->EndFrame();
	}

	// 243x243
	{
		viewport.x = m_FrameWidth*0.75f;
		viewport.y = m_FrameHeight*0.75f;
		scissor.offset.x = m_FrameWidth*0.75f;
		scissor.offset.y = m_FrameHeight*0.50f;

		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		m_DebugLum243x243->BeginFrame();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_DebugLum243x243->GetPipeline());
		m_DebugLum243x243->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_DebugLum243x243->EndFrame();
	}
}

void HDRPipelineDemo::RenderFinal(VkCommandBuffer commandBuffer,int32 backBufferIndex)
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
		float w = (m_Debug ? 0.5f : 1)*m_FrameWidth;
		float h = (m_Debug ? 0.5f : 1)*m_FrameHeight;
		float tx = (m_Debug ? 0.25f : 0)*m_FrameWidth;
		float ty = (m_Debug ? 0.25f : 0)*m_FrameHeight;

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

		m_FinalMaterial->BeginFrame();

		m_FinalMaterial->BeginObject();
		m_FinalMaterial->SetLocalUniform("paramData",&m_ParamData,sizeof(ParamBlock));
		m_FinalMaterial->EndObject();

		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_FinalMaterial->GetPipeline());
		m_FinalMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Quad->meshes[0]->BindDrawCmd(commandBuffer);

		m_FinalMaterial->EndFrame();
	}

	// debug pass
	if(m_Debug)
	{
		RenderPipeline(commandBuffer);
	}

	// ui pass
	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
}

void HDRPipelineDemo::SetupCommandBuffers(int32 backBufferIndex)
{
	VkCommandBuffer commandBuffer = m_CommandBuffers[backBufferIndex];

	VkCommandBufferBeginInfo cmdBeginInfo;
	ZeroVulkanStruct(cmdBeginInfo,VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
	VERIFYVULKANRESULT(vkBeginCommandBuffer(commandBuffer,&cmdBeginInfo));

	SourcePass(commandBuffer);
	BrightPass(commandBuffer);
	BlurHPass(commandBuffer);
	BlurVPass(commandBuffer);
	LuminancePass(commandBuffer);
	RenderFinal(commandBuffer,backBufferIndex);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void HDRPipelineDemo::InitParmas()
{
	m_ParamData.intensity.x = 5.0f;
	m_ParamData.intensity.y = 1.0f; // Exposure
	m_ParamData.intensity.z = 0.0f; // Not use
	m_ParamData.intensity.w = 1.5f; // Bias

	m_ViewCamera.SetPosition(25.0f,15.0f,-20.0f);
	m_ViewCamera.LookAt(0,5.0f,0);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,1500.0f);
}

void HDRPipelineDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void HDRPipelineDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}