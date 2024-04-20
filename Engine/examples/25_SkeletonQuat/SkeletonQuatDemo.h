#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define MAX_BONES 64
class SkeletonQuatDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct BonesTransformBlock
	{
		Vector4 dualQuats[MAX_BONES*2];
		Vector4 debugParam;
	};
public:
	SkeletonQuatDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~SkeletonQuatDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateAnimation(float time,float delta);
	bool UpdateUI(float time,float delta);
	void SetAnimation(int32 index);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();
private:
	bool m_Ready = false;
	DVKCamera m_ViewCamera;
	ModelViewProjectionBlock    m_MVPData;
	BonesTransformBlock         m_BonesData;
	DVKModel* m_RoleModel = nullptr;
	DVKShader* m_RoleShader = nullptr;
	DVKTexture* m_RoleDiffuse = nullptr;
	DVKMaterial* m_RoleMaterial = nullptr;
	ImageGUIContext* m_GUI = nullptr;
	bool                        m_AutoAnimation = true;
	float                       m_AnimDuration = 0.0f;
	float                       m_AnimTime = 0.0f;
	int32                       m_AnimIndex = 0;
};
