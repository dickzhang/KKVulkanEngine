#include "VulkanDrawable.h"

#include "VulkanApplication.h"

VulkanDrawable::VulkanDrawable(VulkanRenderer * parent)
{
	//�Խṹ�����ó�ֵ
	memset(&m_UniformData, 0, sizeof(UniformData));
	memset(&m_VertexBuffer, 0, sizeof(VertexBuffer));
	rendererObj = parent;

	VkSemaphoreCreateInfo presentCompleteSemaphoreCreateInfo;
	presentCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	presentCompleteSemaphoreCreateInfo.pNext = NULL;
	presentCompleteSemaphoreCreateInfo.flags = 0;

	VkSemaphoreCreateInfo drawingCompleteSemaphoreCreateInfo;
	drawingCompleteSemaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	drawingCompleteSemaphoreCreateInfo.pNext = NULL;
	drawingCompleteSemaphoreCreateInfo.flags = 0;

	VulkanDevice * deviceObj = VulkanApplication::GetInstance()->deviceObj;
	//��������������ͬ�����ź���,�ź�������ͬ�����ƿ��Կ���л��ߵ�һ�����еĴ�����ָ�����ִ��
	vkCreateSemaphore(deviceObj->device, &presentCompleteSemaphoreCreateInfo, NULL, &presentCompleteSemaphore);
	vkCreateSemaphore(deviceObj->device, &drawingCompleteSemaphoreCreateInfo, NULL, &drawingCompleteSemaphore);
}

VulkanDrawable::~VulkanDrawable()
{
}

void VulkanDrawable::destroyCommandBuffer()
{
	for(int i = 0; i < vecCmdDraw.size(); i++)
	{
		vkFreeCommandBuffers(deviceObj->device, rendererObj->cmdPool, 1, &vecCmdDraw[i]);
	}
}

void VulkanDrawable::destroySynchronizationObjects()
{
	vkDestroySemaphore(deviceObj->device, presentCompleteSemaphore, NULL);
	vkDestroySemaphore(deviceObj->device, drawingCompleteSemaphore, NULL);
}

void VulkanDrawable::createUniformBuffer()
{
	VkResult  result;
	bool  pass;
	Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
	View = glm::lookAt(
		glm::vec3(10, 3, 10),	// Camera in World Space
		glm::vec3(0, 0, 0),		// and looks at the origin
		glm::vec3(0, -1, 0)		// Head is up
	);
	Model = glm::mat4(1.0f);
	MVP = Projection * View * Model;

	// Create buffer resource states using VkBufferCreateInfo
	VkBufferCreateInfo bufInfo = { };
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = NULL;
	bufInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	bufInfo.size = sizeof(MVP);
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = NULL;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.flags = 0;

	// Use create buffer info and create the buffer objects
	result = vkCreateBuffer(deviceObj->device, &bufInfo, NULL, &m_UniformData.buffer);
	assert(result == VK_SUCCESS);

	// Get the buffer memory requirements
	VkMemoryRequirements memRqrmnt;
	vkGetBufferMemoryRequirements(deviceObj->device, m_UniformData.buffer, &memRqrmnt);

	VkMemoryAllocateInfo memAllocInfo = { };
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = NULL;
	memAllocInfo.memoryTypeIndex = 0;
	memAllocInfo.allocationSize = memRqrmnt.size;

	// Determine the type of memory required 
	// with the help of memory properties
	pass = deviceObj->memoryTypeFromProperties(memRqrmnt.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, &memAllocInfo.memoryTypeIndex);
	assert(pass);

	// Allocate the memory for buffer objects
	result = vkAllocateMemory(deviceObj->device, &memAllocInfo, NULL, &( m_UniformData.memory ));
	assert(result == VK_SUCCESS);

	// Map the GPU memory on to local host
	result = vkMapMemory(deviceObj->device, m_UniformData.memory, 0, memRqrmnt.size, 0, ( void ** )&m_UniformData.pData);
	assert(result == VK_SUCCESS);

	// Copy computed data in the mapped buffer
	memcpy(m_UniformData.pData, &MVP, sizeof(MVP));

	// We have only one Uniform buffer object to update
	m_UniformData.mappedRange.resize(1);

	// Populate the VkMappedMemoryRange data structure
	m_UniformData.mappedRange[0].sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
	m_UniformData.mappedRange[0].memory = m_UniformData.memory;
	m_UniformData.mappedRange[0].offset = 0;
	m_UniformData.mappedRange[0].size = sizeof(MVP);

	// Invalidate the range of mapped buffer in order to make it visible to the host.
	// If the memory property is set with VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
	// then the driver may take care of this, otherwise for non-coherent 
	// mapped memory vkInvalidateMappedMemoryRanges() needs to be called explicitly.
	//ʹ���ӳ�仺����Ч��,ʹ����������˿ɼ�
	vkInvalidateMappedMemoryRanges(deviceObj->device, 1, &m_UniformData.mappedRange[0]);

	// Bind the buffer device memory 
	result = vkBindBufferMemory(deviceObj->device, m_UniformData.buffer, m_UniformData.memory, 0);
	assert(result == VK_SUCCESS);

	// Update the local data structure with uniform buffer for house keeping
	m_UniformData.bufferInfo.buffer = m_UniformData.buffer;
	m_UniformData.bufferInfo.offset = 0;
	m_UniformData.bufferInfo.range = sizeof(MVP);
	m_UniformData.memRqrmnt = memRqrmnt;
}

void VulkanDrawable::createVertexBuffer(const void * vertexData, uint32_t dataSize, uint32_t dataStride, bool useTexture)
{
	VulkanApplication * appObj = VulkanApplication::GetInstance();
	VulkanDevice * deviceObj = appObj->deviceObj;

	VkResult  result;
	bool  pass;

	// Create the Buffer resourece metadata information
	VkBufferCreateInfo bufInfo = { };
	bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufInfo.pNext = NULL;
	bufInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	bufInfo.size = dataSize;
	bufInfo.queueFamilyIndexCount = 0;
	bufInfo.pQueueFamilyIndices = NULL;
	bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	bufInfo.flags = 0;

	// Create the Buffer resource
	result = vkCreateBuffer(deviceObj->device, &bufInfo, NULL, &m_VertexBuffer.buf);
	assert(result == VK_SUCCESS);

	// Get the Buffer resource requirements
	VkMemoryRequirements memRqrmnt;
	vkGetBufferMemoryRequirements(deviceObj->device, m_VertexBuffer.buf, &memRqrmnt);

	// Create memory allocation metadata information
	VkMemoryAllocateInfo allocInfo = { };
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.pNext = NULL;
	allocInfo.memoryTypeIndex = 0;
	allocInfo.allocationSize = memRqrmnt.size;

	// Get the compatible type of memory
	pass = deviceObj->memoryTypeFromProperties(memRqrmnt.memoryTypeBits,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &allocInfo.memoryTypeIndex);
	assert(pass);

	// Allocate the physical backing for buffer resource
	result = vkAllocateMemory(deviceObj->device, &allocInfo, NULL, &( m_VertexBuffer.mem ));
	assert(result == VK_SUCCESS);
	m_VertexBuffer.bufferInfo.range = memRqrmnt.size;
	m_VertexBuffer.bufferInfo.offset = 0;

	// Map the physical device memory region to the host 
	uint8_t * pData;
	result = vkMapMemory(deviceObj->device, m_VertexBuffer.mem, 0, memRqrmnt.size, 0, ( void ** )&pData);
	assert(result == VK_SUCCESS);

	// Copy the data in the mapped memory
	memcpy(pData, vertexData, dataSize);

	// Unmap the device memory
	vkUnmapMemory(deviceObj->device, m_VertexBuffer.mem);

	// Bind the allocated buffer resource to the device memory
	result = vkBindBufferMemory(deviceObj->device, m_VertexBuffer.buf, m_VertexBuffer.mem, 0);
	assert(result == VK_SUCCESS);

	// Once the buffer resource is implemented, its binding points are 
	// stored into the(
	// The VkVertexInputBinding viIpBind, stores the rate at which the information will be
	// injected for vertex input.
	viIpBind.binding = 0;
	viIpBind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
	viIpBind.stride = dataStride;

	// The VkVertexInputAttribute - Description) structure, store 
	// the information that helps in interpreting the data.
	viIpAttrb[0].binding = 0;
	viIpAttrb[0].location = 0;
	viIpAttrb[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
	viIpAttrb[0].offset = 0;

	viIpAttrb[1].binding = 0;
	viIpAttrb[1].location = 1;
	viIpAttrb[1].format = useTexture ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R32G32B32A32_SFLOAT;
	viIpAttrb[1].offset = 16; // After, 4 components - RGBA  each of 4 bytes(32bits)
}

// Creates the descriptor pool, this function depends on - 
// createDescriptorSetLayout()
void VulkanDrawable::createDescriptorPool(bool useTexture)
{
	VkResult  result;
	// Define the size of descriptor pool based on the
	// type of descriptor set being used.
	std::vector<VkDescriptorPoolSize> descriptorTypePool;

	// The first descriptor pool object is of type Uniform buffer
	descriptorTypePool.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});

	// If texture is supported then define second object with 
	// descriptor type to be Image sampler
	if(useTexture)
	{
		descriptorTypePool.push_back(VkDescriptorPoolSize{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1});
	}

	// Populate the descriptor pool state information
	// in the create info structure.
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = { };
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.pNext = NULL;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolCreateInfo.poolSizeCount = ( uint32_t )descriptorTypePool.size();
	descriptorPoolCreateInfo.pPoolSizes = descriptorTypePool.data();

	// Create the descriptor pool using the descriptor 
	// pool create info structure
	result = vkCreateDescriptorPool(deviceObj->device, &descriptorPoolCreateInfo, NULL, &descriptorPool);
	assert(result == VK_SUCCESS);
}

// Create the Uniform resource inside. Create Descriptor set associated resources 
// before creating the descriptor set
void VulkanDrawable::createDescriptorResources()
{
	createUniformBuffer();
}

// Creates the descriptor sets using descriptor pool.
// This function depend on the createDescriptorPool() and createUniformBuffer().
void VulkanDrawable::createDescriptorSet(bool useTexture)
{
	VulkanPipeline * pipelineObj = rendererObj->getPipelineObject();
	VkResult  result;

	// Create the descriptor allocation structure and specify the descriptor 
	// pool and descriptor layout
	VkDescriptorSetAllocateInfo dsAllocInfo[1];
	dsAllocInfo[0].sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	dsAllocInfo[0].pNext = NULL;
	dsAllocInfo[0].descriptorPool = descriptorPool;
	dsAllocInfo[0].descriptorSetCount = 1;
	dsAllocInfo[0].pSetLayouts = descLayout.data();

	// Allocate the number of descriptor sets needs to be produced
	descriptorSet.resize(1);

	// Allocate descriptor sets
	result = vkAllocateDescriptorSets(deviceObj->device, dsAllocInfo, descriptorSet.data());
	assert(result == VK_SUCCESS);

	// Allocate two write descriptors for - 1. MVP and 2. Texture
	VkWriteDescriptorSet writes[2];
	memset(&writes, 0, sizeof(writes));

	// Specify the uniform buffer related 
	// information into first write descriptor
	writes[0] = { };
	writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writes[0].pNext = NULL;
	writes[0].dstSet = descriptorSet[0];
	writes[0].descriptorCount = 1;
	writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	writes[0].pBufferInfo = &m_UniformData.bufferInfo;
	writes[0].dstArrayElement = 0;
	writes[0].dstBinding = 0; // DESCRIPTOR_SET_BINDING_INDEX

	// If texture is used then update the second write descriptor structure
	if(useTexture)
	{
		writes[1] = { };
		writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writes[1].dstSet = descriptorSet[0];
		writes[1].dstBinding = 1; // DESCRIPTOR_SET_BINDING_INDEX
		writes[1].descriptorCount = 1;
		writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		writes[1].pImageInfo = &textures->descsImgInfo;
		writes[1].dstArrayElement = 0;
	}

	// ��һ�±���������µ������������������
	vkUpdateDescriptorSets(deviceObj->device, useTexture ? 2 : 1, writes, 0, NULL);
}

void VulkanDrawable::initViewports(VkCommandBuffer * cmd)
{
	viewport.height = ( float )rendererObj->height;
	viewport.width = ( float )rendererObj->width;
	viewport.minDepth = ( float )0.0f;
	viewport.maxDepth = ( float )1.0f;
	viewport.x = 0;
	viewport.y = 0;
	//ֻ����ˮ�������˶�̬״̬VK_DYNAMIC_STATE_VIEWPORT,����ӿ����ò���Ч
	vkCmdSetViewport(*cmd, 0, NUMBER_OF_VIEWPORTS, &viewport);
}

void VulkanDrawable::initScissors(VkCommandBuffer * cmd)
{
	scissor.extent.width = rendererObj->width;
	scissor.extent.height = rendererObj->height;
	scissor.offset.x = 0;
	scissor.offset.y = 0;
	//ֻ����ˮ��״̬���󴴽���ʱ�������˶�̬״̬VK_DYNAMIC_STATE_SCISSOR,����ӿ����ò���Ч
	vkCmdSetScissor(*cmd, 0, NUMBER_OF_SCISSORS, &scissor);
}

void VulkanDrawable::destroyVertexBuffer()
{
	vkDestroyBuffer(rendererObj->getDevice()->device, m_VertexBuffer.buf, NULL);
	vkFreeMemory(rendererObj->getDevice()->device, m_VertexBuffer.mem, NULL);
}

void VulkanDrawable::destroyUniformBuffer()
{
	vkUnmapMemory(deviceObj->device, m_UniformData.memory);
	vkDestroyBuffer(rendererObj->getDevice()->device, m_UniformData.buffer, NULL);
	vkFreeMemory(rendererObj->getDevice()->device, m_UniformData.memory, NULL);
}

void VulkanDrawable::setTextures(TextureData * tex)
{
	textures = tex;
}

void VulkanDrawable::recordCommandBuffer(int currentImage, VkCommandBuffer * cmdDraw)
{
	VulkanDevice * deviceObj = rendererObj->getDevice();
	VulkanPipeline * pipelineObj = rendererObj->getPipelineObject();

	//���������ɫ��ֵ
	VkClearValue clearValues[2];
	clearValues[0].color.float32[0] = 0.3f;
	clearValues[0].color.float32[1] = 0.3f;
	clearValues[0].color.float32[2] = 0.3f;
	clearValues[0].color.float32[3] = 1.0f;

	// �������/ģ���������ɫֵ
	clearValues[1].depthStencil.depth = 1.0f;
	clearValues[1].depthStencil.stencil = 0;

	// Define the VkRenderPassBeginInfo control structure
	VkRenderPassBeginInfo renderPassBegin;
	renderPassBegin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBegin.pNext = NULL;
	renderPassBegin.renderPass = rendererObj->renderPass;
	renderPassBegin.framebuffer = rendererObj->framebuffers[currentImage];
	renderPassBegin.renderArea.offset.x = 0;
	renderPassBegin.renderArea.offset.y = 0;
	renderPassBegin.renderArea.extent.width = rendererObj->width;
	renderPassBegin.renderArea.extent.height = rendererObj->height;
	renderPassBegin.clearValueCount = 2;
	renderPassBegin.pClearValues = clearValues;

	//��ʼ¼����Ⱦͨ����ʵ��
	vkCmdBeginRenderPass(*cmdDraw, &renderPassBegin, VK_SUBPASS_CONTENTS_INLINE);

	//����ˮ�߶���
	vkCmdBindPipeline(*cmdDraw, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

	//����������
	vkCmdBindDescriptorSets(*cmdDraw, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
		0, 1, descriptorSet.data(), 0, NULL);

	//�󶨶��㻺��
	const VkDeviceSize offsets[1] = {0};
	vkCmdBindVertexBuffers(*cmdDraw, 0, 1, &m_VertexBuffer.buf, offsets);

	//��ʼ���ӿڲ���
	initViewports(cmdDraw);

	//��ʼ���ü�����
	initScissors(cmdDraw);

	// ���ƶ���
	vkCmdDraw(*cmdDraw, 3 * 2 * 6, 1, 0, 0);

	//�����Ⱦͨ��ʵ����¼��
	vkCmdEndRenderPass(*cmdDraw);
}

void VulkanDrawable::prepare()
{
	VulkanDevice * deviceObj = rendererObj->getDevice();
	vecCmdDraw.resize(rendererObj->getSwapChain()->scPublicVars.colorBuffer.size());
	for(int i = 0; i < rendererObj->getSwapChain()->scPublicVars.colorBuffer.size(); i++)
	{
		CommandBufferMgr::allocCommandBuffer(&deviceObj->device, *rendererObj->getCommandPool(), &vecCmdDraw[i]);
		CommandBufferMgr::beginCommandBuffer(vecCmdDraw[i]);
		recordCommandBuffer(i, &vecCmdDraw[i]);
		CommandBufferMgr::endCommandBuffer(vecCmdDraw[i]);
	}
}

void VulkanDrawable::update()
{
	VulkanDevice * deviceObj = rendererObj->getDevice();
	Projection = glm::perspective(glm::radians(60.0f), 1.0f, 0.1f, 1000.0f);
	View = glm::lookAt(
		glm::vec3(0, 0, 5),		// Camera is in World Space
		glm::vec3(0, 0, 0),		// and looks at the origin
		glm::vec3(0, 1, 0)		// Head is up
	);
	Model = glm::mat4(1.0f);
	static float rot = 0;
	rot += .0005f;
	Model = glm::rotate(Model, rot, glm::vec3(0.0, 1.0, 0.0))
		* glm::rotate(Model, rot, glm::vec3(1.0, 1.0, 1.0));

	glm::mat4 MVP = Projection * View * Model;

	VkResult res = vkInvalidateMappedMemoryRanges(deviceObj->device, 1, &m_UniformData.mappedRange[0]);
	assert(res == VK_SUCCESS);
	memcpy(m_UniformData.pData, &MVP, sizeof(MVP));
	res = vkFlushMappedMemoryRanges(deviceObj->device, 1, &m_UniformData.mappedRange[0]);
	assert(res == VK_SUCCESS);
}

void VulkanDrawable::render()
{
	VulkanDevice * deviceObj = rendererObj->getDevice();
	VulkanSwapChain * swapChainObj = rendererObj->getSwapChain();

	uint32_t & currentColorImage = swapChainObj->scPublicVars.currentColorBuffer;
	VkSwapchainKHR & swapChain = swapChainObj->scPublicVars.swapChain;

	VkFence nullFence = VK_NULL_HANDLE;

	//��ȡ��һ�����õĽ�����ͼ�������
	VkResult result = swapChainObj->fpAcquireNextImageKHR(deviceObj->device, swapChain,
		UINT64_MAX, presentCompleteSemaphore, VK_NULL_HANDLE, &currentColorImage);

	VkPipelineStageFlags submitPipelineStages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	VkSubmitInfo submitInfo = { };
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.pNext = NULL;
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = &presentCompleteSemaphore;
	submitInfo.pWaitDstStageMask = &submitPipelineStages;
	submitInfo.commandBufferCount = ( uint32_t )sizeof(&vecCmdDraw[currentColorImage]) / sizeof(VkCommandBuffer);
	submitInfo.pCommandBuffers = &vecCmdDraw[currentColorImage];
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = &drawingCompleteSemaphore;

	// �ύָ���
	CommandBufferMgr::submitCommandBuffer(deviceObj->queue, &vecCmdDraw[currentColorImage], &submitInfo);

	// ��ͼ��չʾ������
	VkPresentInfoKHR present = { };
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.pNext = NULL;
	present.swapchainCount = 1;
	present.pSwapchains = &swapChain;
	present.pImageIndices = &currentColorImage;
	present.pWaitSemaphores = &drawingCompleteSemaphore;
	present.waitSemaphoreCount = 1;
	present.pResults = NULL;

	//��ͼ����н���չʾ
	result = swapChainObj->fpQueuePresentKHR(deviceObj->queue, &present);
	assert(result == VK_SUCCESS);
}

void VulkanDrawable::createDescriptorSetLayout(bool useTexture)
{
	// Define the layout binding information for the descriptor set(before creating it)
	// Specify binding point, shader type(like vertex shader below), count etc.
	VkDescriptorSetLayoutBinding layoutBindings[2];
	layoutBindings[0].binding = 0; // DESCRIPTOR_SET_BINDING_INDEX
	layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	layoutBindings[0].descriptorCount = 1;
	layoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layoutBindings[0].pImmutableSamplers = NULL;

	// If texture is being used then there existing second binding in the fragment shader
	if(useTexture)
	{
		layoutBindings[1].binding = 1; // DESCRIPTOR_SET_BINDING_INDEX
		layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		layoutBindings[1].descriptorCount = 1;
		layoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		layoutBindings[1].pImmutableSamplers = NULL;
	}

	// Specify the layout bind into the VkDescriptorSetLayoutCreateInfo
	// and use it to create a descriptor set layout
	VkDescriptorSetLayoutCreateInfo descriptorLayout = { };
	descriptorLayout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorLayout.pNext = NULL;
	descriptorLayout.bindingCount = useTexture ? 2 : 1;
	descriptorLayout.pBindings = layoutBindings;

	VkResult  result;
	// Allocate required number of descriptor layout objects and  
	// create them using vkCreateDescriptorSetLayout()
	descLayout.resize(1);
	result = vkCreateDescriptorSetLayout(deviceObj->device, &descriptorLayout, NULL, descLayout.data());
	assert(result == VK_SUCCESS);
}

// createPipelineLayout is a virtual function from 
// VulkanDescriptor and defined in the VulkanDrawable class.
// virtual void VulkanDescriptor::createPipelineLayout() = 0;

// Creates the pipeline layout to inject into the pipeline
void VulkanDrawable::createPipelineLayout()
{
	// Create the pipeline layout with the help of descriptor layout.
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = { };
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.pNext = NULL;
	pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
	pPipelineLayoutCreateInfo.pPushConstantRanges = NULL;
	pPipelineLayoutCreateInfo.setLayoutCount = ( uint32_t )descLayout.size();
	pPipelineLayoutCreateInfo.pSetLayouts = descLayout.data();

	VkResult  result;
	result = vkCreatePipelineLayout(deviceObj->device, &pPipelineLayoutCreateInfo, NULL, &pipelineLayout);
	assert(result == VK_SUCCESS);
}
