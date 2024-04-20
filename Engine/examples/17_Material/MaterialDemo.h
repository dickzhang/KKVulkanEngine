#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>
#define NUM_LIGHTS 64

class MaterialDemo :public ModuleBase
{
public:

	struct ViewProjectionBlock
	{
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct AttachmentParamBlock
	{
		float       attachmentIndex;
		float       zNear;
		float       zFar;
		float       one;
		float       xMaxFar;
		float       yMaxFar;
		Vector2     padding;
		Matrix4x4   invView;
	};

	struct PointLight
	{
		Vector4 position;
		Vector3 color;
		float   radius;
	};

	struct LightSpawnBlock
	{
		Vector3 position[NUM_LIGHTS];
		Vector3 direction[NUM_LIGHTS];
		float speed[NUM_LIGHTS];
	};

	struct LightDataBlock
	{
		PointLight lights[NUM_LIGHTS];
	};

public:
	MaterialDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~MaterialDemo();
	virtual bool PreInit() override;
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	virtual void CreateFrameBuffers() override;
	virtual void CreateDepthStencil() override;
	virtual void DestoryDepthStencil() override;
	void DestroyAttachments();
	void CreateAttachments();
	virtual void CreateRenderPass() override;
	virtual void DestroyFrameBuffers() override;
	virtual void DestoryRenderPass() override;
	void Draw(float time,float delta);
	void UpdateUniform(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();
private:
	typedef std::vector<DVKTexture*>           DVKTextureArray;

	bool                            m_Ready = false;
	DVKCamera              m_ViewCamera;

	ViewProjectionBlock             m_ViewProjData;

	AttachmentParamBlock            m_VertFragParam;
	LightDataBlock                  m_LightDatas;
	LightSpawnBlock                 m_LightInfos;

	DVKModel* m_Model = nullptr;

	DVKShader* m_Shader0 = nullptr;
	DVKMaterial* m_Material0 = nullptr;

	DVKShader* m_Shader1 = nullptr;
	DVKMaterial* m_Material1 = nullptr;

	DVKTextureArray                 m_AttachsDepth;
	DVKTextureArray                 m_AttachsColor;
	DVKTextureArray                 m_AttachsNormal;

	ImageGUIContext* m_GUI = nullptr;
};
