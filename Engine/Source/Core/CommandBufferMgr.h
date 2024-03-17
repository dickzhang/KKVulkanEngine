#pragma once
#include "../Common/Headers.h"

/***************COMMAND BUFFER WRAPPERS***************/
class CommandBufferMgr
{
public:
	//从指令池中分配指令缓存
	static void allocCommandBuffer(const VkDevice * device, const VkCommandPool cmdPool, VkCommandBuffer * cmdBuf, const VkCommandBufferAllocateInfo * commandBufferInfo = NULL);
	//开始录制指令缓存
	static void beginCommandBuffer(VkCommandBuffer cmdBuf, VkCommandBufferBeginInfo * inCmdBufInfo = NULL);
	//结束录制指令缓存
	static void endCommandBuffer(VkCommandBuffer cmdBuf);
	//提交指令缓存到队列
	static void submitCommandBuffer(const VkQueue & queue, const VkCommandBuffer * cmdBufList, const VkSubmitInfo * submitInfo = NULL, const VkFence & fence = VK_NULL_HANDLE);
};

void * readFile(const char * spvFileName, size_t * fileSize);

/***************TEXTURE WRAPPERS***************/
struct TextureData
{
	VkSampler				sampler;//设置一个与图像关联的采样器对象
	VkImage					image; //图像对象
	VkImageLayout			imageLayout;//图像布局
	VkMemoryAllocateInfo	memoryAlloc;//存储内存分配的信息
	VkDeviceMemory			mem;//设置为图像资源分配的物理设备内存
	VkImageView				view;//图像视图
	uint32_t				mipMapLevels;//mipmap层级
	uint32_t				layerCount;//图像资源中层的数量
	uint32_t				textureWidth, textureHeight;//图像资源的尺寸
	VkDescriptorImageInfo	descsImgInfo;//设置描述符图像信息
};

/***************PPM PARSER CLASS***************/
#include "../Common/Headers.h"

class PpmParser
{
public:
	PpmParser();
	~PpmParser();
	bool getHeaderInfo(const char * filename);
	bool loadImageData(int rowPitch, uint8_t * rgba_data);
	int32_t getImageWidth();
	int32_t getImageHeight();
	const char * filename()
	{
		return ppmFile.c_str();
	}

private:
	bool isValid;
	int32_t imageWidth;
	int32_t imageHeight;
	int32_t dataPosition;
	std::string ppmFile;
	gli::texture2D * tex2D;
};