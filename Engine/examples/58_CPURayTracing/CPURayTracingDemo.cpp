#include "CPURayTracingDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<CPURayTracingDemo>(1400,900,"CPURayTracingDemo",cmdLine);
}

CPURayTracingDemo::CPURayTracingDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

CPURayTracingDemo::~CPURayTracingDemo()
{

}

bool CPURayTracingDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	CPURayTracing();
	LoadAssets();
	InitParmas();

	m_Ready = true;

	return true;
}

void CPURayTracingDemo::Exist()
{
	DestroyAssets();
	ModuleBase::Release();
}

void CPURayTracingDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}


uint8 CPURayTracingDemo::ToUint8(float f)
{
	if(f<0.0f)
	{
		f = 0.0f;
	}
	if(f>=1.0f)
	{
		f = 1.0f;
	}
	return 255*f;
}

Vector4 CPURayTracingDemo::ToGammaSpace(Vector4 color)
{
	Vector4 ret;
	ret.x = MMath::Pow(color.x,1.0f/2.2f);
	ret.y = MMath::Pow(color.y,1.0f/2.2f);
	ret.z = MMath::Pow(color.z,1.0f/2.2f);
	ret.w = MMath::Pow(color.w,1.0f/2.2f);
	return ret;
}

void CPURayTracingDemo::CPURayTracing()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// camera
	DVKCamera camera;
	camera.Perspective(PI/4,WIDTH,HEIGHT,0.01f,100.0f);

	// tracing
	std::vector<Raytracing*> raytracings;
	raytracings.resize(WIDTH*HEIGHT);

	// scene
	Scene scene;
	scene.spheres.push_back(Sphere(Vector3(0,0,5),0.5f,new DiffuseMaterial(Vector4(0.8f,0.3f,0.3f,1.0f))));
	scene.spheres.push_back(Sphere(Vector3(0,-100.5f,5),100.0f,new MetalMaterial(Vector4(0.8f,0.8f,0.0f,1.0f),0.0f)));
	scene.spheres.push_back(Sphere(Vector3(-1,0,5),0.5f,new MetalMaterial(Vector4(0.8f,0.8f,0.8f,1.0f),0.2f)));
	scene.spheres.push_back(Sphere(Vector3(1,0,5),0.5f,new MetalMaterial(Vector4(0.8f,0.6f,0.2f,1.0f),0.2f)));

	// prepare work
	for(int32 h = 0; h<HEIGHT; ++h)
	{
		for(int32 w = 0; w<WIDTH; ++w)
		{
			int32 index = (h*WIDTH+w);
			raytracings[index] = new Raytracing(&scene,&camera,w,h,index*4,WIDTH,HEIGHT);
		}
	}

	// start work
	TaskThreadPool* taskPool = new TaskThreadPool();
	taskPool->Create(MMath::Max<int32>(std::thread::hardware_concurrency(),8));

	for(int32 i = 0; i<raytracings.size(); ++i)
	{
		taskPool->AddTask(raytracings[i]);
	}

	while(true)
	{
		bool complete = true;
		for(int32 i = 0; i<raytracings.size(); ++i)
		{
			if(!raytracings[i]->complete)
			{
				complete = false;
				break;
			}
		}
		if(complete)
		{
			break;
		}
	}

	// output color
	uint8* rgba = new uint8[WIDTH*HEIGHT*4];

	for(int32 i = 0; i<raytracings.size(); ++i)
	{
		Raytracing* tracing = raytracings[i];
		tracing->color = ToGammaSpace(tracing->color);

		rgba[tracing->index+0] = ToUint8(tracing->color.x);
		rgba[tracing->index+1] = ToUint8(tracing->color.y);
		rgba[tracing->index+2] = ToUint8(tracing->color.z);
		rgba[tracing->index+3] = ToUint8(tracing->color.w);

		delete tracing;
	}

	m_Texture = DVKTexture::Create2D(rgba,WIDTH*HEIGHT*4,VK_FORMAT_R8G8B8A8_UNORM,WIDTH,HEIGHT,m_VulkanDevice,cmdBuffer);
	m_Texture->UpdateSampler(VK_FILTER_LINEAR,VK_FILTER_LINEAR,VK_SAMPLER_MIPMAP_MODE_LINEAR,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

	delete[] rgba;
	delete cmdBuffer;
	delete taskPool;
}

void CPURayTracingDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	SetupGfxCommand(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

void CPURayTracingDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	m_Quad = DVKDefaultRes::fullQuad;

	m_Shader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/44_ComputeRaytracing/Texture.vert.spv",
		"Assets/Shaders/44_ComputeRaytracing/Texture.frag.spv"
	);

	m_Material = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_Shader
	);
	m_Material->pipelineInfo.rasterizationState.cullMode = VK_CULL_MODE_NONE;
	m_Material->PreparePipeline();

	m_SceneModel = DVKModel::LoadFromFile(
		"Assets/models/simplescene.obj",
		m_VulkanDevice,
		nullptr,
		{
			VertexAttribute::VA_Position
		}
	);

	m_Material->SetTexture("diffuseMap",m_Texture);

	delete cmdBuffer;
}

void CPURayTracingDemo::DestroyAssets()
{
	delete m_Texture;
	delete m_Material;
	delete m_Shader;
	delete m_SceneModel;
}

void CPURayTracingDemo::SetupGfxCommand(int32 backBufferIndex)
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
	m_Material->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_Quad->meshes[0]->BindDrawCmd(commandBuffer);
	m_Material->EndFrame();

	vkCmdEndRenderPass(commandBuffer);
	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void CPURayTracingDemo::InitParmas()
{
	DVKCamera camera;
	camera.SetPosition(0,2.5f,-10.0f);
	camera.LookAt(0,2.5f,0);
	camera.Perspective(PI/4,GetWidth(),GetHeight(),1.0f,1500.0f);
}