#include "MeshLodDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<MeshLodDemo>(1400,900,"MeshLodDemo",cmdLine);
}

MeshLodDemo::MeshLodDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

MeshLodDemo::~MeshLodDemo()
{
}

bool MeshLodDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();
	CreateGUI();
	InitParmas();
	LoadAssets();
	m_Ready = true;
	return true;
}

void MeshLodDemo::Exist()
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
}

void MeshLodDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void MeshLodDemo::Draw(float time,float delta)
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

bool MeshLodDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();
	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("MeshLodDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);
		int32 totalCount = m_IndexBuffer->indexCount;
		ImGui::SliderInt("LOD",&m_LodIndex,0,LOD_GROUP_SIZE-1);
		ImGui::Text("Tri:%d\n",m_LodGroups[m_LodIndex].count/3);
		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}
	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();
	m_GUI->EndFrame();
	m_GUI->Update();
	return hovered;
}

void MeshLodDemo::ProcessMesh(DVKCommandBuffer* cmdBuffer)
{
	const uint32 VertexStride = 6;
	std::vector<float> tempVertices;
	std::vector<uint32> tempIndices;
	for(int32 m = 0; m<m_Model->meshes.size(); ++m)
	{
		DVKMesh* mesh = m_Model->meshes[m];
		for(int32 p = 0; p<mesh->primitives.size(); ++p)
		{
			DVKPrimitive* primitive = mesh->primitives[p];
			uint32 indexStart = tempVertices.size()/VertexStride;

			for(int32 i = 0; i<primitive->vertices.size(); ++i)
			{
				tempVertices.push_back(primitive->vertices[i]);
			}

			for(int32 i = 0; i<primitive->indices.size(); ++i)
			{
				tempIndices.push_back(primitive->indices[i]+indexStart);
			}
		}
	}

	// Indexing
	std::vector<uint32> remap;
	remap.resize(tempVertices.size()/VertexStride);
	size_t totalVertices = meshopt_generateVertexRemap(
		remap.data(),
		tempIndices.data(),
		tempIndices.size(),
		tempVertices.data(),
		tempVertices.size()/VertexStride,
		sizeof(float)*VertexStride
	);

	std::vector<uint32> indices;
	indices.resize(tempIndices.size());
	meshopt_remapIndexBuffer(
		indices.data(),
		tempIndices.data(),
		tempIndices.size(),
		remap.data()
	);

	std::vector<float> vertices;
	vertices.resize(totalVertices*VertexStride);
	meshopt_remapVertexBuffer(
		vertices.data(),
		tempVertices.data(),
		tempVertices.size()/VertexStride,
		sizeof(float)*VertexStride,
		remap.data()
	);

	// Vertex cache optimization
	meshopt_optimizeVertexCache(
		indices.data(),
		indices.data(),
		indices.size(),
		vertices.size()/VertexStride
	);

	// Overdraw optimization
	meshopt_optimizeOverdraw(
		indices.data(),
		indices.data(),
		indices.size(),
		vertices.data(),
		vertices.size()/VertexStride,
		sizeof(float)*VertexStride,
		3.0f
	);

	// Vertex fetch optimization
	meshopt_optimizeVertexFetch(
		vertices.data(),
		indices.data(),
		indices.size(),
		vertices.data(),
		vertices.size()/VertexStride,
		sizeof(float)*VertexStride
	);

	// Simplification
	uint32 indexCount = indices.size();

	m_LodGroups[0].start = 0;
	m_LodGroups[0].count = indices.size();

	float bias = 1.0f/LOD_GROUP_SIZE;
	for(int lodIdx = 1; lodIdx<LOD_GROUP_SIZE; ++lodIdx)
	{
		float threshold = 1.0f-lodIdx*bias;
		uint32 lodIndexCount = indexCount*threshold/3*3;

		std::vector<uint32> lodIndices;
		lodIndices.resize(lodIndexCount);

		uint32 realCount = meshopt_simplifySloppy(
			lodIndices.data(),
			indices.data(),
			indexCount,
			vertices.data(),
			vertices.size()/VertexStride,
			sizeof(float)*VertexStride,
			lodIndexCount
		);
		lodIndices.resize(realCount);

		m_LodGroups[lodIdx].start = indices.size();
		m_LodGroups[lodIdx].count = lodIndices.size();

		for(int32 i = 0; i<lodIndices.size(); ++i)
		{
			indices.push_back(lodIndices[i]);
		}
	}

	m_VertexBuffer = DVKVertexBuffer::Create(m_VulkanDevice,cmdBuffer,vertices,{ VertexAttribute::VA_Position,VertexAttribute::VA_Normal });
	m_IndexBuffer = DVKIndexBuffer::Create(m_VulkanDevice,cmdBuffer,indices);
}

void MeshLodDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_Model = DVKModel::LoadFromFile(
		"Assets/models/suzanne.obj",
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
		"Assets/Shaders/72_MeshLOD/obj.vert.spv",
		"Assets/Shaders/72_MeshLOD/obj.frag.spv",
		nullptr
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->PreparePipeline();
	ProcessMesh(cmdBuffer);
	delete cmdBuffer;
}

void MeshLodDemo::DestroyAssets()
{
	delete m_Model;
	delete m_Material;
	delete m_Shader;
	delete m_VertexBuffer;
	delete m_IndexBuffer;
}

void MeshLodDemo::SetupCommandBuffers(int32 backBufferIndex)
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
	m_MVPParam.model.SetIdentity();
	m_MVPParam.model.RotateY(180);
	m_MVPParam.view = m_ViewCamera.GetView();
	m_MVPParam.proj = m_ViewCamera.GetProjection();

	m_Material->BeginObject();
	m_Material->SetLocalUniform("uboMVP",&m_MVPParam,sizeof(ModelViewProjectionBlock));
	m_Material->EndObject();
	m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);

	int32 indexCount = m_LodGroups[m_LodIndex].count;
	int32 indexStart = m_LodGroups[m_LodIndex].start;

	vkCmdBindVertexBuffers(commandBuffer,0,1,&(m_VertexBuffer->dvkBuffer->buffer),&(m_VertexBuffer->offset));
	vkCmdBindIndexBuffer(commandBuffer,m_IndexBuffer->dvkBuffer->buffer,0,m_IndexBuffer->indexType);
	vkCmdDrawIndexed(commandBuffer,indexCount,1,indexStart,0,0);
	m_Material->EndFrame();
	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);
	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void MeshLodDemo::InitParmas()
{
	m_ViewCamera.SetPosition(0,0,-50.0f);
	m_ViewCamera.LookAt(0,0,0);
	m_ViewCamera.Perspective(PI/4,(float)GetWidth(),(float)GetHeight(),1.0f,500.0f);
}

void MeshLodDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void MeshLodDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}