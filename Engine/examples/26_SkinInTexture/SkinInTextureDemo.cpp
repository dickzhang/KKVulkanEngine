#include "SkinInTextureDemo.h"

std::shared_ptr<AppModuleBase> CreateAppMode(const std::vector<std::string>& cmdLine)
{
	return std::make_shared<SkinInTextureDemo>(1400,900,"SkinInTextureDemo",cmdLine);
}

SkinInTextureDemo::SkinInTextureDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine) :
	ModuleBase(width,height,title,cmdLine)
{
}

SkinInTextureDemo::~SkinInTextureDemo()
{
}
bool SkinInTextureDemo::Init()
{
	ModuleBase::Setup();
	ModuleBase::Prepare();

	LoadAssets();
	InitParmas();
	CreateGUI();

	m_Ready = true;

	return true;
}

void SkinInTextureDemo::Exist()
{
	ModuleBase::Release();
	DestroyAssets();
	DestroyGUI();
}

void SkinInTextureDemo::Loop(float time,float delta)
{
	if(!m_Ready)
	{
		return;
	}
	Draw(time,delta);
}

void SkinInTextureDemo::UpdateAnimation(float time,float delta)
{
	if(m_AutoAnimation)
	{
		m_AnimTime += delta;
	}

	if(m_AnimTime>m_RoleModel->GetAnimation(0).duration)
	{
		m_AnimTime = m_AnimTime-m_RoleModel->GetAnimation(0).duration;
	}

	// ���������������
	int32 index = 0;
	for(int32 i = 0; i<m_Keys.size(); ++i)
	{
		if(m_AnimTime<=m_Keys[i])
		{
			index = i;
			break;
		}
	}

	// ������װ�����ǹ����������ǹҽӵ������ϵģ�Ϊ�˸������ǵĶ���������������ĺ�����
	// �Ż����ҽ���Ϣ�����洢�����ظ����㡣������ÿһ֡�����Ѿ���ǰ����ô洢����Texture��
	m_RoleModel->GotoAnimation(m_Keys[index]);

	DVKMesh* mesh = m_RoleModel->meshes[0];

	m_ParamData.animIndex.x = 64;
	m_ParamData.animIndex.y = 32;
	m_ParamData.animIndex.z = index*mesh->bones.size()*2;
	m_ParamData.animIndex.w = 0;
}

void SkinInTextureDemo::Draw(float time,float delta)
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

	// m_RoleModel->rootNode->localMatrix.AppendRotation(delta * 90.0f, Vector3::UpVector);
	m_RoleMaterial->BeginFrame();
	for(int32 i = 0; i<m_RoleModel->meshes.size(); ++i)
	{
		DVKMesh* mesh = m_RoleModel->meshes[i];
		// ����Ƿ�Ϊ��������
		m_ParamData.animIndex.w = mesh->bones.size()==0 ? 0 : 1;

		m_ParamData.model = mesh->linkNode->GetGlobalMatrix();
		m_RoleMaterial->BeginObject();
		m_RoleMaterial->SetLocalUniform("paramData",&m_ParamData,sizeof(ParamDataBlock));
		m_RoleMaterial->EndObject();
	}
	m_RoleMaterial->EndFrame();

	SetupCommandBuffers(bufferIndex);

	ModuleBase::Present(bufferIndex);
}

bool SkinInTextureDemo::UpdateUI(float time,float delta)
{
	m_GUI->StartFrame();

	{
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(0,0),ImGuiSetCond_FirstUseEver);
		ImGui::Begin("SkinInTextureDemo",nullptr,ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove);

		ImGui::Checkbox("AutoPlay",&m_AutoAnimation);

		if(!m_AutoAnimation)
		{
			ImGui::SliderFloat("Time",&m_AnimTime,0.0f,m_AnimDuration);
		}

		ImGui::Text("%.3f ms/frame (%.1f FPS)",1000.0f/ImGui::GetIO().Framerate,ImGui::GetIO().Framerate);
		ImGui::End();
	}

	bool hovered = ImGui::IsAnyWindowHovered()||ImGui::IsAnyItemHovered()||ImGui::IsRootWindowOrAnyChildHovered();

	m_GUI->EndFrame();
	m_GUI->Update();

	return hovered;
}

void SkinInTextureDemo::SetAnimation(int32 index)
{
	m_RoleModel->SetAnimation(index);
	m_AnimDuration = m_RoleModel->animations[index].duration;
	m_AnimTime = 0.0f;
	m_AnimIndex = index;
}

void SkinInTextureDemo::CreateAnimTexture(DVKCommandBuffer* cmdBuffer)
{
	std::vector<float> animData(64*32*4); // 21������ * 30֡�������� * 8
	DVKAnimation& animation = m_RoleModel->GetAnimation();

	// ��ȡ�ؼ�֡��Ϣ
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

	// �洢ÿһ֡����Ӧ�Ķ�������
	for(int32 i = 0; i<m_Keys.size(); ++i)
	{
		m_RoleModel->GotoAnimation(m_Keys[i]);
		// ���ݲ�����һ���ڵ�Ķ���������Ҫ����Vector�洢��
		int32 step = i*mesh->bones.size()*8;

		for(int32 j = 0; j<mesh->bones.size(); ++j)
		{
			int32 boneIndex = mesh->bones[j];
			DVKBone* bone = m_RoleModel->bones[boneIndex];
			// ��ȡ����������Transform����
			// Ҳ����ʹ�ö�ż��Ԫ�����滻����ļ���
			Matrix4x4 boneTransform = bone->finalTransform;
			boneTransform.Append(mesh->linkNode->GetGlobalMatrix().Inverse());
			// ��Transform�����л�ȡ��Ԫ���Լ�λ����Ϣ
			Quat quat = boneTransform.ToQuat();
			Vector3 pos = boneTransform.GetOrigin();
			// תΪʹ�ö�ż��Ԫ��
			float dx = (+0.5)*(pos.x*quat.w+pos.y*quat.z-pos.z*quat.y);
			float dy = (+0.5)*(-pos.x*quat.z+pos.y*quat.w+pos.z*quat.x);
			float dz = (+0.5)*(pos.x*quat.y-pos.y*quat.x+pos.z*quat.w);
			float dw = (-0.5)*(pos.x*quat.x+pos.y*quat.y+pos.z*quat.z);
			// �������ǰ֡��ǰ������Texture�е�����
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

	// ����Texture
	m_AnimTexture = DVKTexture::Create2D(
		(const uint8*)animData.data(),
		animData.size()*sizeof(float),
		VK_FORMAT_R32G32B32A32_SFLOAT,
		64,
		32,
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

void SkinInTextureDemo::LoadAssets()
{
	DVKCommandBuffer* cmdBuffer = DVKCommandBuffer::Create(m_VulkanDevice,m_CommandPool);

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
	m_RoleModel->rootNode->localMatrix.AppendRotation(180,Vector3::UpVector);

	// animation
	SetAnimation(0);
	CreateAnimTexture(cmdBuffer);

	// shader
	m_RoleShader = DVKShader::Create(
		m_VulkanDevice,
		true,
		"Assets/Shaders/29_SkinInTexture/obj.vert.spv",
		"Assets/Shaders/29_SkinInTexture/obj.frag.spv"
	);

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

void SkinInTextureDemo::DestroyAssets()
{
	delete m_RoleShader;
	delete m_RoleDiffuse;
	delete m_RoleMaterial;
	delete m_RoleModel;
	delete m_AnimTexture;
}

void SkinInTextureDemo::SetupCommandBuffers(int32 backBufferIndex)
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
	for(int32 j = 0; j<m_RoleModel->meshes.size(); ++j)
	{
		m_RoleMaterial->BindDescriptorSets(commandBuffer,VK_PIPELINE_BIND_POINT_GRAPHICS,j);
		m_RoleModel->meshes[j]->BindDrawCmd(commandBuffer);
	}

	m_GUI->BindDrawCmd(commandBuffer,m_RenderPass);

	vkCmdEndRenderPass(commandBuffer);

	VERIFYVULKANRESULT(vkEndCommandBuffer(commandBuffer));
}

void SkinInTextureDemo::InitParmas()
{
	DVKBoundingBox bounds = m_RoleModel->rootNode->GetBounds();
	Vector3 boundSize = bounds.max-bounds.min;
	Vector3 boundCenter = bounds.min+boundSize*0.5f;

	m_ViewCamera.SetPosition(boundCenter.x,boundCenter.y,boundCenter.z-boundSize.Size()*2.0);
	m_ViewCamera.Perspective(PI/4,GetWidth(),GetHeight(),0.10f,3000.0f);
}

void SkinInTextureDemo::CreateGUI()
{
	m_GUI = new ImageGUIContext();
	m_GUI->Init("Assets/fonts/Ubuntu-Regular.ttf");
}

void SkinInTextureDemo::DestroyGUI()
{
	m_GUI->Destroy();
	delete m_GUI;
}