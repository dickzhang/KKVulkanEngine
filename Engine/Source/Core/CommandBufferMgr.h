#pragma once
#include "../Common/Headers.h"

/***************COMMAND BUFFER WRAPPERS***************/
class CommandBufferMgr
{
public:
	//��ָ����з���ָ���
	static void allocCommandBuffer(const VkDevice * device, const VkCommandPool cmdPool, VkCommandBuffer * cmdBuf, const VkCommandBufferAllocateInfo * commandBufferInfo = NULL);
	//��ʼ¼��ָ���
	static void beginCommandBuffer(VkCommandBuffer cmdBuf, VkCommandBufferBeginInfo * inCmdBufInfo = NULL);
	//����¼��ָ���
	static void endCommandBuffer(VkCommandBuffer cmdBuf);
	//�ύָ��浽����
	static void submitCommandBuffer(const VkQueue & queue, const VkCommandBuffer * cmdBufList, const VkSubmitInfo * submitInfo = NULL, const VkFence & fence = VK_NULL_HANDLE);
};

void * readFile(const char * spvFileName, size_t * fileSize);

/***************TEXTURE WRAPPERS***************/
struct TextureData
{
	VkSampler				sampler;//����һ����ͼ������Ĳ���������
	VkImage					image; //ͼ�����
	VkImageLayout			imageLayout;//ͼ�񲼾�
	VkMemoryAllocateInfo	memoryAlloc;//�洢�ڴ�������Ϣ
	VkDeviceMemory			mem;//����Ϊͼ����Դ����������豸�ڴ�
	VkImageView				view;//ͼ����ͼ
	uint32_t				mipMapLevels;//mipmap�㼶
	uint32_t				layerCount;//ͼ����Դ�в������
	uint32_t				textureWidth, textureHeight;//ͼ����Դ�ĳߴ�
	VkDescriptorImageInfo	descsImgInfo;//����������ͼ����Ϣ
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