#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class ShuffleIntrinsicsDemo :public ModuleBase
{
public:
	ShuffleIntrinsicsDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ShuffleIntrinsicsDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                        m_Ready = false;
	DVKModel* m_Model = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKMaterial* m_Material = nullptr;
	Vector4                     m_Type;
	ImageGUIContext* m_GUI = nullptr;
};
