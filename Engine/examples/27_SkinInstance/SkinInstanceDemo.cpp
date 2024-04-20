#include "SkinInstanceDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<SkinInstanceDemo>(1400,900,"SkinInstanceDemo",cmdLine);
}

SkinInstanceDemo::SkinInstanceDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

SkinInstanceDemo::~SkinInstanceDemo()
{
}
bool SkinInstanceDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	LoadAssets();
	InitParmas();
	CreateGUI();

	m_Ready = true;

	return true;
}

void SkinInstanceDemo::Exist()
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
}

void SkinInstanceDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void SkinInstanceDemo::UpdateAnimation(float time,float delta)
{
	if(m_AutoAnimation)
	{
		m_AnimTime += delta;
	}

	if(m_AnimTime>m_RoleModel->GetAnimation(0).duration)
	{
		m_AnimTime = m_AnimTime-m_RoleModel->GetAnimation(0).duration;
	}

	// 计算出动画的索引
	int32 index = 0;
	for(int32 i = 0; i<m_Keys.size(); ++i)
	{
		if(m_AnimTime<=m_Keys[i])
		{
			index = i;
			break;
		}
	}

	DVKMesh* mesh = m_RoleModel->meshes[0];

	m_ParamData.animIndex.x = m_AnimTexture->width;
	m_ParamData.animIndex.y = m_AnimTexture->height;
	m_ParamData.animIndex.z = index*mesh->bones.size()*2;
	m_ParamData.animIndex.w = m_Keys.size()*mesh->bones.size()*2;
}

void SkinInstanceDemo::Draw(float time,float delta)
{
	int32 bufferIndex = ModuleBase::AcquireBackbufferIndex();

	bool hovered = UpdateUI(time,delta);
	if(!hovered)
	{
		m_ViewCamera.Update(time,delta);
	}

	m_ParamData.view = m_ViewCamera.GetView();
	m_ParamData.projection = m_ViewCamera.GetProjection();

	UpdateAnimation(time,delta);

	m_RoleMaterial->BeginFrame();

	DVKMesh* mesh = m_RoleModel->meshes[0];
	m_ParamData.model = mesh->linkNode->GetGlobalMatrix();

	m_RoleMaterial->BeginObject();
	m_RoleMaterial->SetLocalUniform("paramData",&m_ParamData,sizeof(ParamDataBlock));
	m_RoleMaterial->EndObject();
	m_RoleMaterial->EndFrame();

	SetupCommandBuffers(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool SkinInstanceDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	m_FrameCounter += 1;
	m_LastFrameTime += delta;
	if(m_LastFrameTime>=1.0f)
	{
		m_LastFPS = m_FrameCounter;
		m_FrameCounter = 0;
		m_LastFrameTime = 0.0f;
	}

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("SkinInTextureDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		DVKPrimitive* primitive = m_RoleModel->meshes[0]->primitives[0];

		ImGui::SliderInt("Instance",&(primitive->indexBuffer->instanceCount),1,INSTANCE_COUNT);

		ImGui::Checkbox("AutoPlay",&m_AutoAnimation);

		if(!m_AutoAnimation)
		{
			ImGui::SliderFloat("Time",&m_AnimTime,0.0f,m_AnimDuration);
		}

		ImGui::Text("DrawCall:1");
		ImGui::Text("%.3f ms/frame (%d FPS)",1000.0f/m_LastFPS,m_LastFPS);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void SkinInstanceDemo::SetAnimation(int32 index)
{
	m_RoleModel->SetAnimation(index);
	m_AnimDuration = m_RoleModel->animations[index].duration;
	m_AnimTime = 0.0f;
	m_AnimIndex = index;
}

void SkinInstanceDemo::CreateAnimTexture(DVKCommandBuffer* cmdBuffer)
{
	std::vector<float> animData(64*64*4); // 21个骨骼 * 30帧动画数据 * 8
	DVKAnimation& animation = m_RoleModel->GetAnimation();

	// 获取关键帧信息
	m_Keys.push_back(0);
	for(auto it = animation.clips.begin(); it!=animation.clips.end(); ++it)
	{
		DVKAnimationClip& clip = it->second;
		for(int32 i = 0; i<clip.positions.keys.size(); ++i)
		{
			if(m_Keys.back()<clip.positions.keys[i])
			{
				m_Keys.push_back(clip.positions.keys[i]);
			}
		}
		for(int32 i = 0; i<clip.rotations.keys.size(); ++i)
		{
			if(m_Keys.back()<clip.rotations.keys[i])
			{
				m_Keys.push_back(clip.rotations.keys[i]);
			}
		}
		for(int32 i = 0; i<clip.scales.keys.size(); ++i)
		{
			if(m_Keys.back()<clip.scales.keys[i])
			{
				m_Keys.push_back(clip.scales.keys[i]);
			}
		}
	}

	DVKMesh* mesh = m_RoleModel->meshes[0];

	// 存储每一帧所对应的动画数据
	for(int32 i = 0; i<m_Keys.size(); ++i)
	{
		m_RoleModel->GotoAnimation(m_Keys[i]);
		// 数据步长，一个节点的动画数据需要两个Vector存储。
		int32 step = i*mesh->bones.size()*8;

		for(int32 j = 0; j<mesh->bones.size(); ++j)
		{
			int32 boneIndex = mesh->bones[j];
			DVKBone* bone = m_RoleModel->bones[boneIndex];
			// 获取骨骼的最终Transform矩阵
			// 也可以使用对偶四元素来替换矩阵的计算
			Matrix4x4 boneTransform = bone->finalTransform;
			boneTransform.Append(mesh->linkNode->GetGlobalMatrix().Inverse());
			boneTransform.AppendRotation(180,Vector3::ForwardVector);
			// 从Transform矩阵中获取四元数以及位移信息
			Quat quat = boneTransform.ToQuat();
			Vector3 pos = boneTransform.GetOrigin();
			// 转为使用对偶四元数
			float dx = (+0.5)*(pos.x*quat.w+pos.y*quat.z-pos.z*quat.y);
			float dy = (+0.5)*(-pos.x*quat.z+pos.y*quat.w+pos.z*quat.x);
			float dz = (+0.5)*(pos.x*quat.y-pos.y*quat.x+pos.z*quat.w);
			float dw = (-0.5)*(pos.x*quat.x+pos.y*quat.y+pos.z*quat.z);
			// 计算出当前帧当前骨骼在Texture中的坐标
			int32 index = step+j*8;
			animData[index+0] = quat.x;
			animData[index+1] = quat.y;
			animData[index+2] = quat.z;
			animData[index+3] = quat.w;
			animData[index+4] = dx;
			animData[index+5] = dy;
			animData[index+6] = dz;
			animData[index+7] = dw;
		}
	}

	// 创建Texture
	m_AnimTexture = DVKTexture::Create2D(
		(const uint8*)animData.data(),
		animData.size()*sizeof(float),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		64,
		64,
		m_VulkanDevice,
		cmdBuffer
	);
	m_AnimTexture->UpdateSampler(
		VK_FILTER_NEAREST,
		VK_FILTER_NEAREST,
		VK_SAMPLER_MIPMAP_MODE_NEAREST,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE
	);
}

void SkinInstanceDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

	// shader
	m_RoleShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/30_SkinInstance/obj.vert.spv",
		"Assets/Shaders/30_SkinInstance/obj.frag.spv"
	);

	// model
	m_RoleModel = DVKModel::LoadFromFile(
		"Assets/models/xiaonan/nvhai.fbx",
		m_VulkanDevice,
		cmdBuffer,
		{
			VertexAttribute::VA_Position,
			VertexAttribute::VA_UV0,
			VertexAttribute::VA_Normal,
			VertexAttribute::VA_SkinPack,
		}
	);

	// animation
	SetAnimation(0);
	CreateAnimTexture(cmdBuffer);

	// instance data
	DVKMesh* mesh = m_RoleModel->meshes[0];
	Matrix4x4 meshGlobal = mesh->linkNode->GetGlobalMatrix();
	DVKPrimitive* primitive = m_RoleModel->meshes[0]->primitives[0];
	primitive->instanceDatas.resize(9*INSTANCE_COUNT);

	for(int32 i = 0; i<INSTANCE_COUNT; ++i)
	{
		Vector3 translate;
		translate.x = MMath::RandRange(-300.0f,300.0f);
		translate.y = MMath::RandRange(-180.0f,180.0f);
		translate.z = MMath::RandRange(-150.0f,150.0f);

		Matrix4x4 matrix = meshGlobal;
		matrix.AppendRotation(MMath::RandRange(0.0f,360.0f),Vector3::UpVector);
		matrix.AppendTranslation(translate);

		Quat quat = matrix.ToQuat();
		Vector3 pos = matrix.GetOrigin();
		float dx = (+0.5)*(pos.x*quat.w+pos.y*quat.z-pos.z*quat.y);
		float dy = (+0.5)*(-pos.x*quat.z+pos.y*quat.w+pos.z*quat.x);
		float dz = (+0.5)*(pos.x*quat.y-pos.y*quat.x+pos.z*quat.w);
		float dw = (-0.5)*(pos.x*quat.x+pos.y*quat.y+pos.z*quat.z);

		int32 index = i*9;
		primitive->instanceDatas[index+0] = quat.x;
		primitive->instanceDatas[index+1] = quat.y;
		primitive->instanceDatas[index+2] = quat.z;
		primitive->instanceDatas[index+3] = quat.w;
		primitive->instanceDatas[index+4] = dx;
		primitive->instanceDatas[index+5] = dy;
		primitive->instanceDatas[index+6] = dz;
		primitive->instanceDatas[index+7] = dw;
		primitive->instanceDatas[index+8] = MMath::RandRange(0,m_Keys.size())*mesh->bones.size()*2;
	}

	primitive->indexBuffer->instanceCount = 1024;
	primitive->instanceBuffer = DVKVertexBuffer::Create(m_VulkanDevice,cmdBuffer,primitive->instanceDatas,m_RoleShader->instancesAttributes);

	// texture
	m_RoleDiffuse = DVKTexture::Create2D(
		"Assets/models/xiaonan/b001.jpg",
		m_VulkanDevice,
		cmdBuffer
	);

	// material
	m_RoleMaterial = DVKMaterial::Create(
		m_VulkanDevice,
		m_RenderPass,
		m_PipelineCache,
		m_RoleShader
	);
	m_RoleMaterial->PreparePipeline();
	m_RoleMaterial->SetTexture("diffuseMap",m_RoleDiffuse);
	m_RoleMaterial->SetTexture("animMap",m_AnimTexture);

	delete cmdBuffer;
}

void SkinInstanceDemo::DestroyAssets()
{
	delete m_RoleShader;
	delete m_RoleDiffuse;
	delete m_RoleMaterial;
	delete m_RoleModel;
	delete m_AnimTexture;
}

void SkinInstanceDemo::SetupCommandBuffers(int32 backBufferIndex)
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

	vkCmdBindPipeline(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,m_RoleMaterial->GetPipeline());
	m_RoleMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,0);
	m_RoleModel->meshes[0]->BindDrawCmd(commandBuffer);

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void SkinInstanceDemo::InitParmas()
{
	DVKBoundingBox bounds = m_RoleModel->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;

	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y,boundCenter.z-boundSize.Size()*10.0);
	m_ViewCamera.Perspective(PI/4,GetWidth(),GetHeight(),0.10f,3000.0f);
}

void SkinInstanceDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void SkinInstanceDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}