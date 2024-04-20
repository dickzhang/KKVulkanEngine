#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define INSTANCE_COUNT 20480

class InstanceDrawDemo :public ModuleBase
{
public:
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};
public:
	InstanceDrawDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~InstanceDrawDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateAnim(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();
private:
	bool                        m_Ready = false;
	DVKCamera          m_ViewCamera;
	ModelViewProjectionBlock    m_MVPData;
	DVKModel* m_RoleModel = nullptr;
	DVKShader* m_RoleShader = nullptr;
	DVKMaterial* m_RoleMaterial = nullptr;
	DVKTexture* m_RoleTexture = nullptr;
	bool                        m_AutoSpin = false;
	ImageGUIContext* m_GUI = nullptr;
};
