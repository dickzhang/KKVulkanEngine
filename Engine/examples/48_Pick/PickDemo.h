#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

class PickDemo :public ModuleBase
{
public:
	struct SimpleLine
	{
		std::vector<float> vertices;
		Vector3 lastVert;
		int32 length;
		int32 index;

		SimpleLine()
		{

		}

		void Resize(int32 size)
		{
			length = size;
			index = 0;
			vertices.resize(size);
		}

		void Clear()
		{
			index = 0;
			memset(vertices.data(),0,vertices.size()*sizeof(float));
		}

		void MoveTo(float x,float y,float z)
		{
			lastVert.x = x;
			lastVert.y = y;
			lastVert.z = z;
		}

		void LineTo(float x,float y,float z)
		{
			if(index+6>=length)
			{
				return;
			}

			vertices[index++] = lastVert.x;
			vertices[index++] = lastVert.y;
			vertices[index++] = lastVert.z;

			vertices[index++] = x;
			vertices[index++] = y;
			vertices[index++] = z;

			MoveTo(x,y,z);
		}
	};
	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 proj;
	};
public:
	PickDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~PickDemo();
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	bool UpdateUI(float time,float delta);
	bool IntersectTriangle(const Vector3& orig,const Vector3& dir,Vector3& v0,Vector3& v1,Vector3& v2,float* t,float* u,float* v);
	void UpdateLine(float time,float delta);
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	bool                        m_Ready = false;
	SimpleLine                  m_SimpleLine;
	DVKBuffer* m_ModelLine = nullptr;
	DVKMaterial* m_MaterialLine = nullptr;
	DVKShader* m_ShaderLine = nullptr;
	DVKModel* m_Model = nullptr;
	DVKMaterial* m_Material = nullptr;
	DVKShader* m_Shader = nullptr;
	DVKCamera          m_ViewCamera;
	ModelViewProjectionBlock    m_MVPParam;
	ImageGUIContext* m_GUI = nullptr;
};
