#pragma once
#include "Common/Common.h"
#include "Common/Log.h"
#include "Graphics/DVKCommon.h"
#include "Math/Vector4.h"
#include "Math/Matrix4x4.h"
#include "Loader/ImageLoader.h"
#include <vector>

enum ImageFilterType
{
	FilterNormal = 0,
	Filter3x3Convolution,
	FilterBilateralBlur,
	FilterBrightness,
	FilterBulgeDistortion,
	FilterCGAColorspace,
	FilterColorBalance,
	FilterColorInvert,
	FilterColorMatrix,
	FilterContrast,
	FilterCrosshatch,
	FilterDirectionalSobelEdgeDetection,
	FilterExposure,
	FilterFalseColor,
	FilterGamma,
	FilterGlassSphere,
	FilterGrayscale,
	FilterHalftone,
	FilterHaze,
	FilterHighlightShadow,
	FilterHue,
	FilterKuwahara,
	FilterLevels,
	FilterLuminance,
	FilterLuminanceThreshold,
	FilterMonochrome,
	FilterPixelation,
	FilterPosterize,
	FilterSharpen,
	FilterSolarize,
	FilterSphereRefraction,
	FilterCount
};
class RenderTargetDemo :public ModuleBase
{
public:
	struct FilterItem
	{
		DVKMaterial* material;
		DVKShader* shader;
		ImageFilterType         type;

		void Create(const char* vert,const char* frag,std::shared_ptr<VulkanDevice> vulkanDevice,VkRenderPass renderPass,VkPipelineCache pipelineCache,DVKTexture* rtt)
		{
			shader = DVKShader::Create(
				vulkanDevice,
				true,
				vert,
				frag
			);
			material = DVKMaterial::Create(
				vulkanDevice,
				renderPass,
				pipelineCache,
				shader
			);
			material->PreparePipeline();
			material->SetTexture("inputImageTexture",rtt);
		}

		void Destroy()
		{
			delete material;
			delete shader;
		}
	};

	struct ModelViewProjectionBlock
	{
		Matrix4x4 model;
		Matrix4x4 view;
		Matrix4x4 projection;
	};

	struct Filter3x3ConvolutionParamBlock
	{
		float       texelWidth;
		float       texelHeight;
		float       lineSize = 1;
		float       padding0;
		float       convolutionMatrix[16] = {
			-0.1f,1.0f,1.0f,0.0f,
			-0.15f,0.0f,-0.6f,0.0f,
			-0.2f,0.0f,-1.0f,0.0f,
			0.0f,0.0f,0.0f,0.0f
		};
	} filter3x3ConvolutionParam;

	struct FilterBilateralBlurParamBlock
	{
		Vector2  singleStepOffset;
		float    distanceNormalizationFactor;
		float    padding0;
	} filterBilateralBlurParam;

	struct FilterBrightnessParamBlock
	{
		float   brightness;
		Vector3 padding;
	} filterBrightnessParam;

	struct FilterBulgeDistortionParamBlock
	{
		float   aspectRatio;
		float   radius;
		float   scale;
		float   padding;
		Vector4 center;
	} filterBulgeDistortionParam;

	struct FilterColorBalanceParamBlock
	{
		Vector4 shadowsShift;
		Vector4 midtonesShift;
		Vector4 highlightsShift;
		int     preserveLuminosity;
		Vector3 padding;
	} filterColorBalanceParam;

	struct FilterColorMatrixParamBlock
	{
		Matrix4x4 colorMatrix;
		float     intensity;
		Vector3   padding;
	} filterColorMatrixParam;

	struct FilterContrastParamBlock
	{
		float   contrast;
		Vector3 padding;
	} filterContrastParam;

	struct FilterCrosshatchParamBlock
	{
		float crossHatchSpacing;
		float lineWidth;
		float padding1;
		float padding2;
	} filterCrosshatchParam;

	struct FilterDirectionalSobelEdgeDetectionParamBlock
	{
		float  texelWidth;
		float  texelHeight;
		float  lineSize;
		float  padding;
	} filterDirectionalSobelEdgeDetectionParam;

	struct FilterExposureParamBlock
	{
		float   exposure;
		Vector3 padding;
	} filterExposureParam;

	struct FilterFalseColorParamBlock
	{
		Vector4 firstColor;
		Vector4 secondColor;
	} filterFalseColorParam;

	struct FilterGammaParamBlock
	{
		Vector4 gamma;
	} filterGammaParam;

	struct FilterGlassSphereParamBlock
	{
		Vector4 center;
		float   radius;
		float   aspectRatio;
		float   refractiveIndex;
		float   padding;
	} filterGlassSphereParam;

	struct FilterHalftoneParamBlock
	{
		float fractionalWidthOfPixel;
		float aspectRatio;
		float padding0;
		float padding1;
	} filterHalftoneParam;

	struct FilterHazeParamBlock
	{
		float distance;
		float slope;
		float padding0;
		float padding1;
	} filterHazeParam;

	struct FilterHighlightShadowParamBlock
	{
		float   shadows;
		float   highlights;
		Vector2 padding;
	} filterHighlightShadowParam;

	struct FilterHueParamBlock
	{
		float   hueAdjust;
		float   hue;
		Vector2 padding;
	} filterHueParam;

	struct FilterKuwaharaParamBlock
	{
		int   radius;
		float padding0;
		float padding1;
		float padding2;
	} filterKuwaharaParam;

	struct FilterLevelsParamBlock
	{
		Vector4 levelMinimum;
		Vector4 levelMiddle;
		Vector4 levelMaximum;
		Vector4 minOutput;
		Vector4 maxOutput;
	} filterLevelsParam;

	struct FilterLuminanceThresholdParamBlock
	{
		float   threshold;
		Vector3 padding;
	} filterLuminanceThresholdParam;

	struct FilterMonochromeParamBlock
	{
		Vector3 filterColor;
		float   intensity;
	} filterMonochromeParam;

	struct FilterPixelationParamBlock
	{
		float imageWidthFactor;
		float imageHeightFactor;
		float pixel;
		float padding;
	} filterPixelationParam;

	struct FilterPosterizeParamBlock
	{
		float colorLevels;
		Vector3 padding;
	} filterPosterizeParam;

	struct FilterSharpenParamBlock
	{
		float imageWidthFactor;
		float imageHeightFactor;
		float sharpness;
		float padding2;
	} filterSharpenParam;

	struct FilterSolarizeParamBlock
	{
		float threshold;
		float padding0;
		float padding1;
		float padding2;
	} filterSolarizeParam;

	struct FilterSphereRefractionParamBlock
	{
		Vector4 center;
		float   radius;
		float   aspectRatio;
		float   refractiveIndex;
		float   padding;
	} filterSphereRefractionParam;

	struct FrameBufferObject
	{
		int32                   width = 0;
		int32                   height = 0;

		VkDevice                device = VK_NULL_HANDLE;
		VkFramebuffer           frameBuffer = VK_NULL_HANDLE;
		VkRenderPass            renderPass = VK_NULL_HANDLE;

		DVKTexture* color = nullptr;
		DVKTexture* depth = nullptr;

		void Destroy()
		{
			if(color)
			{
				delete color;
				color = nullptr;
			}

			if(depth)
			{
				delete depth;
				depth = nullptr;
			}

			if(frameBuffer!=VK_NULL_HANDLE)
			{
				vkDestroyFramebuffer(device,frameBuffer,VULKAN_CPU_ALLOCATOR);
				frameBuffer = VK_NULL_HANDLE;
			}

			if(renderPass!=VK_NULL_HANDLE)
			{
				vkDestroyRenderPass(device,renderPass,VULKAN_CPU_ALLOCATOR);
				renderPass = VK_NULL_HANDLE;
			}
		}
	};

public:
	RenderTargetDemo(int32 width,int32 height,const char* title,const std::vector<std::string>& cmdLine);
	~RenderTargetDemo();
	virtual bool PreInit() override;
	virtual bool Init() override;
	virtual void Exist() override;
	virtual void Loop(float time,float delta) override;
	void Draw(float time,float delta);
	void UpdateFilterParams(float time,float delta);
	void UpdateFilter3x3ConvolutionUI();
	void UpdateFilterBilateralBlurUI();
	void UpdateFilterBrightnessUI();
	void UpdateFilterBulgeDistortionUI();
	void UpdateFilterColorBalanceUI();
	void UpdateFilterColorMatrixUI();
	void UpdateFilterContrastUI();
	void UpdateFilterCrosshatchUI();
	void UpdateFilterDirectionalSobelEdgeDetectionUI();
	void UpdateFilterExposureUI();
	void UpdateFilterFalseColorUI();
	void UpdateFilterGammaUI();
	void UpdateFilterGlassSphereUI();
	void UpdateFilterHalftoneUI();
	void UpdateFilterHazeUI();
	void UpdateFilterHighlightShadowUI();
	void UpdateFilterHueUI();
	void UpdateFilterKuwaharaUI();
	void UpdateFilterLevelsUI();
	void UpdateFilterLuminanceThresholdUI();
	void UpdateFilterMonochromeUI();
	void UpdateFilterPixelationUI();
	void UpdateFilterPosterizeUI();
	void UpdateFilterSharpenUI();
	void UpdateFilterSolarizeUI();
	void UpdateFilterSphereRefractionUI();
	void UpdateFilterUI(float time,float delta);
	bool UpdateUI(float time,float delta);
	void CreateRenderTarget();
	void DestroyRenderTarget();
	void LoadAssets();
	void DestroyAssets();
	void SetupCommandBuffers(int32 backBufferIndex);
	void InitParmas();
	void CreateGUI();
	void DestroyGUI();

private:
	typedef std::vector<DVKTexture*>           TextureArray;
	typedef std::vector<DVKMaterial*>          MaterialArray;
	typedef std::vector<std::vector<DVKMesh*>> MatMeshArray;

	bool                        m_Ready = false;

	DVKCamera          m_ViewCamera;

	FrameBufferObject           m_RenderTarget;

	DVKModel* m_Quad = nullptr;

	ModelViewProjectionBlock    m_MVPData;
	DVKModel* m_ModelScene = nullptr;
	DVKShader* m_SceneShader = nullptr;
	TextureArray                m_SceneDiffuses;
	MaterialArray               m_SceneMaterials;
	MatMeshArray                m_SceneMatMeshes;

	std::vector<const char*>    m_FilterNames;
	std::vector<const char*>    m_FilterSpirvs;
	std::vector<FilterItem>     m_FilterItems;
	int32                       m_Selected = 0;

	ImageGUIContext* m_GUI = nullptr;
};
