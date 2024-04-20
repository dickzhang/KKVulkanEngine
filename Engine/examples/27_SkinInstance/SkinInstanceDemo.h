#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define INSTANCE_COUNT 8000
class SkinInstanceDemo :public ModuleBase
{
public:
	struct ParamDataBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
		Vector4 animIndex;
	};

	struct InstanceData
	{
		Vector4 quat0;
		Vector4 quat1;
	};
public:
	SkinInstanceDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~SkinInstanceDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateAnimation(float time,float delta);
	bool UpdateUI(float time,float delta);
	void SetAnimation(int32 index);
	void CreateAnimTexture(DVKCommandBuffer* cmdBuffer);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:

	bool                        m_Ready = false;
	DVKCamera          m_ViewCamera;

	ParamDataBlock              m_ParamData;

	DVKModel* m_RoleModel = nullptr;
	DVKShader* m_RoleShader = nullptr;
	DVKTexture* m_RoleDiffuse = nullptr;
	DVKMaterial* m_RoleMaterial = nullptr;

	ImageGUIContext* m_GUI = nullptr;

	DVKTexture* m_AnimTexture = nullptr;
	std::vector<float>          m_Keys;
	bool                        m_AutoAnimation = true;
	float                       m_AnimDuration = 0.0f;
	float                       m_AnimTime = 0.0f;
	int32                       m_AnimIndex = 0;

	int32                       m_FrameCounter = 0;
	float                       m_LastFrameTime = 0.0f;
	float                       m_LastFPS = 0.0f;
};
