#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

#define PARTICLE_COUNT (1024 * 1024)

class ComputeParticlesDemo :public ModuleBase
{
public:
	struct ParticleVertex
	{
		Vector4 position;
		Vector4 velocity;
	};

	struct ParticleParam
	{
		Vector4 data0;
		Vector4 data1;
	};
public:
	ComputeParticlesDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~ComputeParticlesDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupComputeCommand();
	void SetupGfxCommand(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                            m_Ready = false;
	DVKBuffer* m_ParticleBuffer = nullptr;
	DVKShader* m_ParticleShader = nullptr;
	DVKMaterial* m_ParticleMaterial = nullptr;

	DVKTexture* m_GradientTexture = nullptr;
	DVKTexture* m_DiffuseTexture = nullptr;

	DVKShader* m_ComputeShader = nullptr;
	DVKCompute* m_ComputeProcessor = nullptr;
	DVKCommandBuffer* m_ComputeCommand = nullptr;

	ParticleParam                   m_ParticleParams;
	int32                           m_PointCount = 0;
	bool                            m_Animation = false;

	ImageGUIContext* m_GUI = nullptr;
};
