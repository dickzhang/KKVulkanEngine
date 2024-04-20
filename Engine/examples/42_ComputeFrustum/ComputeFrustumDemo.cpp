#include "ComputeFrustumDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<ComputeFrustumDemo>(1400,900,"ComputeFrustumDemo",cmdLine);
}

ComputeFrustumDemo::ComputeFrustumDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

ComputeFrustumDemo::~ComputeFrustumDemo()
{

}

bool ComputeFrustumDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CreateGUI();
	InitParmas();
	LoadAssets();

	m_Ready = true;

	return true;
}

void ComputeFrustumDemo::Exist()
{
	DestroyAssets();
	DestroyGUI();
	ModuleBase::Release();
}

void ComputeFrustumDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void ComputeFrustumDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	UpdateFPS(time,delta);
	bool hovered = UpdateUI(time,delta);

	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
		UpdateFrustumPlanes();
	}

	m_DrawCall = 0;

	if(m_UseGPU)
	{
		SetupComputeCommand();
	}

	SetupGfxCommand(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool ComputeFrustumDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("ComputeFrustumDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Checkbox("Compute",&m_UseGPU);
		ImGui::Text("DrawCall:%d",m_DrawCall);

		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void ComputeFrustumDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_ModelSphere = DVKModel::LoadFromFile(
		"Assets/models/sphere.obj",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_Normal
		}
	);
	auto bounds = m_ModelSphere->rootNode->GetBounds();
	m_Radius = bounds.max.x-bounds.min.x;

	for(int32 i = 0; i<1024; ++i)
	{
		m_ObjModels[i].AppendTranslation(
			Vector3(
				MMath::FRandRange(-450.0f,450.0f),
				MMath::FRandRange(-100.0f,100.0f),
				MMath::FRandRange(-450.0f,450.0f)
			)
		);
	}
	for(int32 i = 1024; i<OBJECT_COUNT; ++i)
	{
		m_ObjModels[i].AppendTranslation(
			Vector3(
				MMath::FRandRange(-100000.0f,100000.0f),
				MMath::FRandRange(-100000.0f,100000.0f),
				MMath::FRandRange(-100000.0f,100000.0f)
			)
		);
	}

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/45_ComputeFrustum/Solid.vert.spv",
		"Assets/Shaders/45_ComputeFrustum/Solid.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();

	{
		m_CullingBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			OBJECT_COUNT*sizeof(Vector4)
		);
		m_CullingBuffer->Map();

		DVKBuffer* stagingBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			OBJECT_COUNT*sizeof(Matrix4x4),
			m_ObjModels
		);

		m_MatrixBuffer = DVKBuffer::CreateBuffer(
			m_VulkanDevice,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			OBJECT_COUNT*sizeof(Matrix4x4)
		);

		cmdBuffer->Begin();

		VkBufferCopy copyRegion = { };
		copyRegion.size = OBJECT_COUNT*sizeof(Matrix4x4);
		vkCmdCopyBuffer(cmdBuffer->cmdBuffer,stagingBuffer->buffer,m_MatrixBuffer->buffer,1,&copyRegion);

		cmdBuffer->End();
		cmdBuffer->Submit();

		delete stagingBuffer;
	}

	m_ComputeShader = DVKShader::Create(
		m_VulkanDevice,
		"Assets/Shaders/45_ComputeFrustum/Frustum.comp.spv"
	);

	m_ComputeProcessor = DVKCompute::Create(
		m_VulkanDevice,
		m_PipelineCache,
		m_ComputeShader
	);
	m_ComputeProcessor->SetStorageBuffer("inMatrix",m_MatrixBuffer);
	m_ComputeProcessor->SetStorageBuffer("outCulling",m_CullingBuffer);

	m_ComputeCommand = DVKCommandBuffer::Create(
		m_VulkanDevice,
		m_ComputeCommandPool,
		VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		m_VulkanDevice->GetComputeQueue()
	);

	m_FrustumParam.count.x = OBJECT_COUNT;
	m_FrustumParam.count.y = m_Radius;

	delete cmdBuffer;
}

void ComputeFrustumDemo::DestroyAssets()
{
	delete m_ModelSphere;

	delete m_MatrixBuffer;
	delete m_CullingBuffer;

	delete m_Material;
	delete m_Shader;

	delete m_ComputeShader;
	delete m_ComputeProcessor;
	delete m_ComputeCommand;
}

bool ComputeFrustumDemo::IsInFrustum(int32 index)
{
	if(m_UseGPU)
	{
		Vector4* cullData = (Vector4*)m_CullingBuffer->mapped;
		return cullData[index].x>0.0f;
	}
	else
	{
		Vector3 pos = m_ObjModels[index].GetOrigin();

		for(int32 i = 0; i<6; ++i)
		{
			Vector4& plane = m_FrustumParam.frustumPlanes[i];
			float projDist = (plane.x*pos.x)+(plane.y*pos.y)+(plane.z*pos.z)+plane.w+m_Radius;
			if(projDist<=0)
			{
				return false;
			}
		}

		return true;
	}
}

void ComputeFrustumDemo::RenderSpheres(VkCommandBuffer commandBuffer,DVKCamera& camera)
{
	m_Material->BeginFrame();

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_Material->GetPipeline());
	m_ModelSphere->meshes[0]->BindOnly(commandBuffer);

	int32 count = 0;
	for(int32 i = 0; i<OBJECT_COUNT; ++i)
	{
		if(IsInFrustum(i))
		{
			m_MVPParam.model = m_ObjModels[i];
			m_MVPParam.view = camera.GetView();
			m_MVPParam.proj = camera.GetProjection();

			m_Material->BeginObject();
			m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
			m_Material->EndObject();

			m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,count);
			m_ModelSphere->meshes[0]->DrawOnly(commandBuffer);

			count++;
			m_DrawCall += 1;
		}
	}

	m_Material->EndFrame();
}

void ComputeFrustumDemo::SetupComputeCommand()
{
	m_ComputeCommand->Begin();

	m_ComputeProcessor->SetUniform("paramData",&m_FrustumParam,sizeof(FrustumParamBlock));
	m_ComputeProcessor->BindDispatch(m_ComputeCommand->cmdBuffer,32,32,1);

	m_ComputeCommand->Submit();
}

void ComputeFrustumDemo::SetupGfxCommand(int32 backBufferIndex)
{
	VkViewport viewport = { };
	viewport.x = 0;
	viewport.y = m_FrameHeight;
	viewport.width = m_FrameWidth;
	viewport.height = -(float)m_FrameHeight/2;    // flip y axis
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = { };
	scissor.extent.width = m_FrameWidth;
	scissor.extent.height = m_FrameHeight/2;
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

	// normal
	{
		viewport.y = m_FrameHeight*0.5f;
		scissor.offset.y = 0;
		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		RenderSpheres(commandBuffer,m_ViewCamera);
	}

	// occlusion view
	{
		viewport.y = m_FrameHeight;
		scissor.offset.y = m_FrameHeight*0.5f;
		vkCmdSetViewport(commandBuffer,0,1,&viewport);
		vkCmdSetScissor(commandBuffer,0,1,&scissor);

		RenderSpheres(commandBuffer,m_TopCamera);
	}

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void ComputeFrustumDemo::UpdateFrustumPlanes()
{
	Matrix4x4 matrix = m_ViewCamera.GetViewProjection();

	Vector4* frustumPlanes = &(m_FrustumParam.frustumPlanes[0]);

	// left
	frustumPlanes[0].x = matrix.m[0][3]+matrix.m[0][0];
	frustumPlanes[0].y = matrix.m[1][3]+matrix.m[1][0];
	frustumPlanes[0].z = matrix.m[2][3]+matrix.m[2][0];
	frustumPlanes[0].w = matrix.m[3][3]+matrix.m[3][0];

	// right
	frustumPlanes[1].x = matrix.m[0][3]-matrix.m[0][0];
	frustumPlanes[1].y = matrix.m[1][3]-matrix.m[1][0];
	frustumPlanes[1].z = matrix.m[2][3]-matrix.m[2][0];
	frustumPlanes[1].w = matrix.m[3][3]-matrix.m[3][0];

	// top
	frustumPlanes[2].x = matrix.m[0][3]+matrix.m[0][1];
	frustumPlanes[2].y = matrix.m[1][3]+matrix.m[1][1];
	frustumPlanes[2].z = matrix.m[2][3]+matrix.m[2][1];
	frustumPlanes[2].w = matrix.m[3][3]+matrix.m[3][1];

	// bottom
	frustumPlanes[3].x = matrix.m[0][3]-matrix.m[0][1];
	frustumPlanes[3].y = matrix.m[1][3]-matrix.m[1][1];
	frustumPlanes[3].z = matrix.m[2][3]-matrix.m[2][1];
	frustumPlanes[3].w = matrix.m[3][3]-matrix.m[3][1];

	// near
	frustumPlanes[4].x = matrix.m[0][2];
	frustumPlanes[4].y = matrix.m[1][2];
	frustumPlanes[4].z = matrix.m[2][2];
	frustumPlanes[4].w = matrix.m[3][2];

	// far
	frustumPlanes[5].x = matrix.m[0][3]-matrix.m[0][2];
	frustumPlanes[5].y = matrix.m[1][3]-matrix.m[1][2];
	frustumPlanes[5].z = matrix.m[2][3]-matrix.m[2][2];
	frustumPlanes[5].w = matrix.m[3][3]-matrix.m[3][2];

	for(auto i = 0; i<6; i++)
	{
		float length = MMath::Sqrt(frustumPlanes[i].x*frustumPlanes[i].x+frustumPlanes[i].y*frustumPlanes[i].y+frustumPlanes[i].z*frustumPlanes[i].z);
		frustumPlanes[i].x /= length;
		frustumPlanes[i].y /= length;
		frustumPlanes[i].z /= length;
		frustumPlanes[i].w /= length;
	}
}

void ComputeFrustumDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,19.73f,-200.0f);
	m_ViewCamera.LookAt(0,19.73f,0);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight()*0.5f,1.0f,1500.0f);

	m_TopCamera.SetPosition(-500,1500,0);
	m_TopCamera.LookAt(0,0,0);
	m_TopCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight()*0.5f,1.0f,3000.0f);
}

void ComputeFrustumDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void ComputeFrustumDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}