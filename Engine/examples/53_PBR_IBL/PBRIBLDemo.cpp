#include "PBRIBLDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<PBRIBLDemo>(1400,900,"PBRIBLDemo",cmdLine);
}

PBRIBLDemo::PBRIBLDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

PBRIBLDemo::~PBRIBLDemo()
{

}

bool PBRIBLDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	LoadEnvAssets();
	GenEnvIrradiance();
	GenEnvBRDFLut();
	GenEnvPrefiltered();
	LoadModelAssets();
	InitParmas();

	m_Ready = true;
	return true;
}

void PBRIBLDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void PBRIBLDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void PBRIBLDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	m_PBRParam.cameraPos = m_ViewCamera.GetTransform().GetOrigin();

	SetupCommandBuffers(bufferIndex);
	ModuleBase::Present(bufferIndex);
}

bool PBRIBLDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("PBRIBLDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Separator();

		ImGui::SliderFloat("AO",&m_PBRParam.param.x,0.0f,1.0f);
		ImGui::SliderFloat("Roughness",&m_PBRParam.param.y,0.0f,1.0f);
		ImGui::SliderFloat("Metallic",&m_PBRParam.param.z,0.0f,1.0f);

		ImGui::Separator();

		ImGui::ColorEdit3("LightColor",(float*)(&m_PBRParam.lightColor));
		ImGui::SliderFloat("Intensity",&m_PBRParam.lightColor.w,0.0f,100.0f);

		ImGui::Separator();

		ImGui::SliderFloat("Exposure",&m_PBRParam.envParam.w,0.0f,10.0f);

		ImGui::Separator();

		int32 debug = m_PBRParam.param.w;
		const char* models[6] = {
			"None",
			"Albedo",
			"Normal",
			"Occlusion",
			"Metallic",
			"Roughness"
		};
		ImGui::Combo("Debug",&debug,models,6);
		m_PBRParam.param.w = debug;

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void PBRIBLDemo::LoadEnvAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_EnvModel = DVKModel::LoadFromFile(
		"Assets/models/cube.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position
		}
	);

	m_EnvTexture = DVKTexture::CreateCube(
		{
			"Assets/textures/cubemap/output_skybox_posx.hdr",
			"Assets/textures/cubemap/output_skybox_negx.hdr",
			"Assets/textures/cubemap/output_skybox_posy.hdr",
			"Assets/textures/cubemap/output_skybox_negy.hdr",
			"Assets/textures/cubemap/output_skybox_posz.hdr",
			"Assets/textures/cubemap/output_skybox_negz.hdr"
		},
		m_VulkanDevice,
		cmdBuffer
	);

	m_EnvShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/56_PBR_IBL/skybox.vert.spv",
		"Assets/Shaders/56_PBR_IBL/skybox.frag.spv"
	);

	m_EnvMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_EnvShader
	);
	m_EnvMaterial->pipelineInfo.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	m_EnvMaterial->pipelineInfo.depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	m_EnvMaterial->pipelineInfo.depthStencilState.depthTestEnable = VK_FALSE;
	m_EnvMaterial->pipelineInfo.depthStencilState.depthWriteEnable = VK_FALSE;
	m_EnvMaterial->pipelineInfo.depthStencilState.stencilTestEnable = VK_FALSE;
	m_EnvMaterial->PreparePipeline();
	m_EnvMaterial->SetTexture("diffuseMap",m_EnvTexture);

	delete cmdBuffer;
}

void PBRIBLDemo::LoadModelAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_Model = DVKModel::LoadFromFile(
		"Assets/models/leather-shoes/model.fbx",
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
		"Assets/models/leather-shoes/RootNode_baseColor.jpg",
		m_VulkanDevice,
		cmdBuffer
	);

	m_TexNormal = DVKTexture::Create2D(
		"Assets/models/leather-shoes/RootNode_normal.jpg",
		m_VulkanDevice,
		cmdBuffer
	);

	m_TexORMParam = DVKTexture::Create2D(
		"Assets/models/leather-shoes/RootNode_occlusionRoughnessMetallic.jpg",
		m_VulkanDevice,
		cmdBuffer
	);

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/56_PBR_IBL/obj.vert.spv",
		"Assets/Shaders/56_PBR_IBL/obj.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();
	m_Material->SetTexture("texAlbedo",m_TexAlbedo);
	m_Material->SetTexture("texNormal",m_TexNormal);
	m_Material->SetTexture("texORMParam",m_TexORMParam);
	m_Material->SetTexture("envIrradiance",m_EnvIrradiance);
	m_Material->SetTexture("envBRDFLut",m_EnvBRDFLut);
	m_Material->SetTexture("envPrefiltered",m_EnvPrefiltered);

	delete cmdBuffer;
}

void PBRIBLDemo::GenEnvPrefiltered()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	int32 envSize = 512;

	m_EnvPrefiltered = DVKTexture::CreateCube(
		m_VulkanDevice,
		cmdBuffer,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		envSize,
		envSize,
		true,
		VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		ImageLayoutBarrier::PixelShaderRead
	);

	DVKTexture* tempTexture = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R16G16B16A16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		envSize,
		envSize,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT
	);

	DVKRenderPassInfo rttInfo(
		tempTexture,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
	);
	DVKRenderTarget* tempRenderTarget = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	tempRenderTarget->colorLayout = ImageLayoutBarrier::TransferSource;

	DVKShader* shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/56_PBR_IBL/skybox.vert.spv",
		"Assets/Shaders/56_PBR_IBL/prefiltered.frag.spv"
	);

	DVKMaterial* material = DVKMaterial::Create(
		m_VulkanDevice,
		tempRenderTarget->GetRenderPass(),
		m_PipelineCache,
		shader
	);
	material->pipelineInfo.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	material->pipelineInfo.depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	material->pipelineInfo.depthStencilState.depthTestEnable = VK_FALSE;
	material->pipelineInfo.depthStencilState.depthWriteEnable = VK_FALSE;
	material->pipelineInfo.depthStencilState.stencilTestEnable = VK_FALSE;
	material->PreparePipeline();
	material->SetTexture("environmentMap",m_EnvTexture);

	DVKCamera camera;
	camera.SetPosition(0,0,0);
	camera.Perspective(PI/2,envSize,envSize,0.10f,3000.0f);

	Vector3 viewTargets[6] = {
		Vector3(1,0,0),
		Vector3(-1,0,0),
		Vector3(0,1,0),
		Vector3(0,-1,0),
		Vector3(0,0,1),
		Vector3(0,0,-1)
	};

	{
		VkImageSubresourceRange subresourceRange = { };
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = m_EnvPrefiltered->mipLevels;
		subresourceRange.layerCount = 6;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.baseMipLevel = 0;

		{
			cmdBuffer->Begin();

			ImagePipelineBarrier(
				cmdBuffer->cmdBuffer,
				m_EnvPrefiltered->image,
				ImageLayoutBarrier::PixelShaderRead,
				ImageLayoutBarrier::TransferDest,
				subresourceRange
			);

			cmdBuffer->Submit();
		}

		VkViewport viewport = { };
		viewport.x = 0;
		viewport.y = envSize;
		viewport.width = envSize;
		viewport.height = -envSize;    // flip y axis
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor = { };
		scissor.extent.width = envSize;
		scissor.extent.height = envSize;
		scissor.offset.x = 0;
		scissor.offset.y = 0;

		for(int32 mip = 0; mip<m_EnvPrefiltered->mipLevels; ++mip)
		{
			float rtSize = envSize*std::pow(0.5f,mip);

			Vector4 params;
			params.x = mip*1.0f/(m_EnvPrefiltered->mipLevels-1.0f);
			params.y = envSize;

			for(int32 face = 0; face<6; ++face)
			{
				cmdBuffer->Begin();

				tempRenderTarget->BeginRenderPass(cmdBuffer->cmdBuffer);

				viewport.x = 0;
				viewport.y = rtSize;
				viewport.width = rtSize;
				viewport.height = -rtSize;

				scissor.extent.width = rtSize;
				scissor.extent.height = rtSize;

				vkCmdSetViewport(cmdBuffer->cmdBuffer,0,1,&viewport);
				vkCmdSetScissor(cmdBuffer->cmdBuffer,0,1,&scissor);

				vkCmdBindPipeline(cmdBuffer->cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,material->GetPipeline());

				camera.SetPosition(Vector3(0,0,0));
				camera.LookAt(viewTargets[face]);

				m_MVPParam.model.SetIdentity();
				m_MVPParam.view = camera.GetView();
				m_MVPParam.proj = camera.GetProjection();

				material->BeginFrame();
				material->BeginObject();
				material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
				material->SetLocalUniform("uboParam",&params,sizeof(Vector4));
				material->EndObject();
				material->EndFrame();

				material->BindDescriptorSets(cmdBuffer->cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
				m_EnvModel->meshes[0]->BindDrawCmd(cmdBuffer->cmdBuffer);

				tempRenderTarget->EndRenderPass(cmdBuffer->cmdBuffer);

				VkImageCopy copyRegion = { };

				copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.srcSubresource.baseArrayLayer = 0;
				copyRegion.srcSubresource.mipLevel = 0;
				copyRegion.srcSubresource.layerCount = 1;

				copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				copyRegion.dstSubresource.baseArrayLayer = face;
				copyRegion.dstSubresource.mipLevel = mip;
				copyRegion.dstSubresource.layerCount = 1;

				copyRegion.extent.width = rtSize;
				copyRegion.extent.height = rtSize;
				copyRegion.extent.depth = 1;

				vkCmdCopyImage(
					cmdBuffer->cmdBuffer,
					tempTexture->image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					m_EnvPrefiltered->image,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					1,
					&copyRegion
				);

				cmdBuffer->Submit();
			}
		}

		{
			cmdBuffer->Begin();

			ImagePipelineBarrier(
				cmdBuffer->cmdBuffer,
				m_EnvPrefiltered->image,
				ImageLayoutBarrier::TransferDest,
				ImageLayoutBarrier::PixelShaderRead,
				subresourceRange
			);

			cmdBuffer->Submit();
		}
	}

	m_PBRParam.envParam.x = m_EnvPrefiltered->width;
	m_PBRParam.envParam.y = m_EnvPrefiltered->mipLevels;

	delete shader;
	delete material;
	delete tempRenderTarget;
	delete tempTexture;
	delete cmdBuffer;
}

void PBRIBLDemo::GenEnvIrradiance()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	int32 envSize = 64;

	m_EnvIrradiance = DVKTexture::CreateCube(
		m_VulkanDevice,
		cmdBuffer,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		envSize,
		envSize,
		false,
		VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_DST_BIT,
		VK_SAMPLE_COUNT_1_BIT,
		ImageLayoutBarrier::PixelShaderRead
	);

	DVKTexture* tempTexture = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R32G32B32A32_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		envSize,
		envSize,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT
	);

	DVKRenderPassInfo rttInfo(
		tempTexture,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
	);
	DVKRenderTarget* tempRenderTarget = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);
	tempRenderTarget->colorLayout = ImageLayoutBarrier::TransferSource;

	DVKShader* shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/56_PBR_IBL/skybox.vert.spv",
		"Assets/Shaders/56_PBR_IBL/irradiance.frag.spv"
	);

	DVKMaterial* material = DVKMaterial::Create(
		m_VulkanDevice,
		tempRenderTarget->GetRenderPass(),
		m_PipelineCache,
		shader
	);
	material->pipelineInfo.rasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	material->pipelineInfo.depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	material->pipelineInfo.depthStencilState.depthTestEnable = VK_FALSE;
	material->pipelineInfo.depthStencilState.depthWriteEnable = VK_FALSE;
	material->pipelineInfo.depthStencilState.stencilTestEnable = VK_FALSE;
	material->PreparePipeline();
	material->SetTexture("environmentMap",m_EnvTexture);

	DVKCamera camera;
	camera.SetPosition(0,0,0);
	camera.Perspective(PI/2,envSize,envSize,0.10f,3000.0f);

	Vector3 viewTargets[6] = {
		Vector3(1,0,0),
		Vector3(-1,0,0),
		Vector3(0,1,0),
		Vector3(0,-1,0),
		Vector3(0,0,1),
		Vector3(0,0,-1)
	};

	// generate env irradiancee
	{
		VkImageSubresourceRange subresourceRange = { };
		subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		subresourceRange.levelCount = 1;
		subresourceRange.layerCount = 6;
		subresourceRange.baseArrayLayer = 0;
		subresourceRange.baseMipLevel = 0;

		{
			cmdBuffer->Begin();

			ImagePipelineBarrier(
				cmdBuffer->cmdBuffer,
				m_EnvIrradiance->image,
				ImageLayoutBarrier::PixelShaderRead,
				ImageLayoutBarrier::TransferDest,
				subresourceRange
			);

			cmdBuffer->Submit();
		}

		for(int32 face = 0; face<6; ++face)
		{
			cmdBuffer->Begin();

			tempRenderTarget->BeginRenderPass(cmdBuffer->cmdBuffer);

			vkCmdBindPipeline(cmdBuffer->cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,material->GetPipeline());

			camera.SetPosition(Vector3(0,0,0));
			camera.LookAt(viewTargets[face]);

			m_MVPParam.model.SetIdentity();
			m_MVPParam.view = camera.GetView();
			m_MVPParam.proj = camera.GetProjection();

			material->BeginFrame();
			material->BeginObject();
			material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
			material->EndObject();
			material->EndFrame();

			material->BindDescriptorSets(cmdBuffer->cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
			m_EnvModel->meshes[0]->BindDrawCmd(cmdBuffer->cmdBuffer);

			tempRenderTarget->EndRenderPass(cmdBuffer->cmdBuffer);

			VkImageCopy copyRegion = { };

			copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.srcSubresource.baseArrayLayer = 0;
			copyRegion.srcSubresource.mipLevel = 0;
			copyRegion.srcSubresource.layerCount = 1;

			copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			copyRegion.dstSubresource.baseArrayLayer = face;
			copyRegion.dstSubresource.mipLevel = 0;
			copyRegion.dstSubresource.layerCount = 1;

			copyRegion.extent.width = envSize;
			copyRegion.extent.height = envSize;
			copyRegion.extent.depth = 1;

			vkCmdCopyImage(
				cmdBuffer->cmdBuffer,
				tempTexture->image,
				VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				m_EnvIrradiance->image,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1,
				&copyRegion
			);

			cmdBuffer->Submit();
		}

		{
			cmdBuffer->Begin();

			ImagePipelineBarrier(
				cmdBuffer->cmdBuffer,
				m_EnvIrradiance->image,
				ImageLayoutBarrier::TransferDest,
				ImageLayoutBarrier::PixelShaderRead,
				subresourceRange
			);

			cmdBuffer->Submit();
		}
	}

	delete shader;
	delete material;
	delete tempRenderTarget;
	delete tempTexture;
	delete cmdBuffer;
}

void PBRIBLDemo::GenEnvBRDFLut()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	DVKModel* quad = DVKDefaultRes::fullQuad;

	m_EnvBRDFLut = DVKTexture::CreateRenderTarget(
		m_VulkanDevice,
		VK_FORMAT_R16G16_SFLOAT,
		VK_IMAGE_ASPECT_COLOR_BIT,
		512,
		512,
		VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT|VK_IMAGE_USAGE_SAMPLED_BIT|VK_IMAGE_USAGE_TRANSFER_SRC_BIT
	);

	DVKRenderPassInfo rttInfo(
		m_EnvBRDFLut,VK_ATTACHMENT_LOAD_OP_CLEAR,VK_ATTACHMENT_STORE_OP_STORE,nullptr
	);
	DVKRenderTarget* tempRenderTarget = DVKRenderTarget::Create(m_VulkanDevice,rttInfo);

	DVKShader* shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/56_PBR_IBL/brdflut.vert.spv",
		"Assets/Shaders/56_PBR_IBL/brdflut.frag.spv"
	);

	DVKMaterial* material = DVKMaterial::Create(
		m_VulkanDevice,
		tempRenderTarget->GetRenderPass(),
		m_PipelineCache,
		shader
	);
	material->pipelineInfo.depthStencilState.depthCompareOp = VK_COMPARE_OP_ALWAYS;
	material->pipelineInfo.depthStencilState.depthTestEnable = VK_FALSE;
	material->pipelineInfo.depthStencilState.depthWriteEnable = VK_FALSE;
	material->pipelineInfo.depthStencilState.stencilTestEnable = VK_FALSE;
	material->PreparePipeline();

	{
		cmdBuffer->Begin();
		tempRenderTarget->BeginRenderPass(cmdBuffer->cmdBuffer);

		vkCmdBindPipeline(cmdBuffer->cmdBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,material->GetPipeline());

		quad->meshes[0]->BindDrawCmd(cmdBuffer->cmdBuffer);

		tempRenderTarget->EndRenderPass(cmdBuffer->cmdBuffer);
		cmdBuffer->Submit();
	}

	delete cmdBuffer;
	delete shader;
	delete material;
	delete tempRenderTarget;
}

void PBRIBLDemo::DestroyAssets()
{
	delete m_Model;

	delete m_Shader;
	delete m_Material;

	delete m_TexAlbedo;
	delete m_TexNormal;
	delete m_TexORMParam;

	delete m_EnvModel;
	delete m_EnvMaterial;
	delete m_EnvShader;
	delete m_EnvTexture;

	delete m_EnvIrradiance;
	delete m_EnvBRDFLut;
	delete m_EnvPrefiltered;
}

void PBRIBLDemo::SetupCommandBuffers(int32 backBufferIndex)
{
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

	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_EnvMaterial->GetPipeline());

		m_MVPParam.model.SetIdentity();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.view.SetOrigin(Vector3(0,0,0));
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_EnvMaterial->BeginFrame();
		m_EnvMaterial->BeginObject();
		m_EnvMaterial->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_EnvMaterial->SetLocalUniform("uboParam",&m_PBRParam,sizeof(PBRParamBlock));
		m_EnvMaterial->EndObject();
		m_EnvMaterial->EndFrame();

		m_EnvMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_EnvModel->meshes[0]->BindDrawCmd(commandBuffer);
	}

	for(int32 i = 0; i<m_Model->meshes.size(); ++i)
	{
		vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());

		m_Material->BeginFrame();

		m_MVPParam.model = m_Model->meshes[i]->linkNode->GetGlobalMatrix();
		m_MVPParam.view = m_ViewCamera.GetView();
		m_MVPParam.proj = m_ViewCamera.GetProjection();

		m_Material->BeginObject();
		m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
		m_Material->SetLocalUniform("uboParam",&m_PBRParam,sizeof(PBRParamBlock));
		m_Material->EndObject();

		m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
		m_Model->meshes[i]->BindDrawCmd(commandBuffer);

		m_Material->EndFrame();
	}

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void PBRIBLDemo::InitParmas()
{
	DVKBoundingBox bounds = m_Model->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;

	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y,boundCenter.z-500.0f);
	m_ViewCamera.LookAt(boundCenter);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),0.10f,3000.0f);

	m_PBRParam.param.x = 1.0f; // ao
	m_PBRParam.param.y = 0.5f; // roughness
	m_PBRParam.param.z = 1.0f; // metallic
	m_PBRParam.param.w = 0.0f; // debug

	m_PBRParam.cameraPos = m_ViewCamera.GetTransform().GetOrigin();

	m_PBRParam.lightColor = Vector4(1,1,1,10.0);

	m_PBRParam.envParam.x = 512;
	m_PBRParam.envParam.y = 9;
	m_PBRParam.envParam.z = 0;
	m_PBRParam.envParam.w = 4.5;
}

void PBRIBLDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void PBRIBLDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}