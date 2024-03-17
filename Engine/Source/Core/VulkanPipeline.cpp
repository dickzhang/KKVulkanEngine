#include "VulkanPipeline.h"
#include "VulkanApplication.h"
#include "VulkanShader.h"
#include "VulkanRenderer.h"
#include "VulkanDevice.h"

VulkanPipeline::VulkanPipeline()
{
	appObj = VulkanApplication::GetInstance();
	deviceObj = appObj->deviceObj;
}

VulkanPipeline::~VulkanPipeline()
{
}
//创建流水线缓冲对象
void VulkanPipeline::createPipelineCache()
{
	VkResult  result;
	VkPipelineCacheCreateInfo pipelineCacheInfo;
	pipelineCacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheInfo.pNext = NULL;
	pipelineCacheInfo.initialDataSize = 0;
	pipelineCacheInfo.pInitialData = NULL;
	pipelineCacheInfo.flags = 0;
	result = vkCreatePipelineCache(deviceObj->device, &pipelineCacheInfo, NULL, &pipelineCache);
	assert(result == VK_SUCCESS);
}

bool VulkanPipeline::createPipeline(VulkanDrawable * drawableObj, VkPipeline * pipeline, VulkanShader * shaderObj, VkBool32 includeDepth, VkBool32 includeVi)
{
	//动态状态的部分
	//VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	VkDynamicState dynamicStateEnables[2];
	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	
	VkPipelineDynamicStateCreateInfo dynamicState = { };
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.pNext = NULL;
	dynamicState.pDynamicStates = dynamicStateEnables;
	dynamicState.dynamicStateCount = 0;

	//顶点输入状态的部分
	VkPipelineVertexInputStateCreateInfo vertexInputStateInfo = { };
	vertexInputStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputStateInfo.pNext = NULL;
	vertexInputStateInfo.flags = 0;
	if(includeVi)
	{
		vertexInputStateInfo.vertexBindingDescriptionCount = sizeof(drawableObj->viIpBind) / sizeof(VkVertexInputBindingDescription);
		vertexInputStateInfo.pVertexBindingDescriptions = &drawableObj->viIpBind;
		vertexInputStateInfo.vertexAttributeDescriptionCount = sizeof(drawableObj->viIpAttrb) / sizeof(VkVertexInputAttributeDescription);
		vertexInputStateInfo.pVertexAttributeDescriptions = drawableObj->viIpAttrb;
	}
	//输入装配状态的部分
	VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo = { };
	inputAssemblyInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyInfo.pNext = NULL;
	inputAssemblyInfo.flags = 0;
	inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;//是否开启图元重启功能
	inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;//图元类型

	//片元是图元拓扑经过光栅化阶段之后的产物
	//光栅化状态的部分
	VkPipelineRasterizationStateCreateInfo rasterStateInfo = { };
	rasterStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterStateInfo.pNext = NULL;
	rasterStateInfo.flags = 0;
	rasterStateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterStateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterStateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterStateInfo.depthClampEnable = includeDepth;
	rasterStateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterStateInfo.depthBiasEnable = VK_FALSE;
	rasterStateInfo.depthBiasConstantFactor = 0;
	rasterStateInfo.depthBiasClamp = 0;
	rasterStateInfo.depthBiasSlopeFactor = 0;
	rasterStateInfo.lineWidth = 1.0f;

	//颜色融混附件的部分
	VkPipelineColorBlendAttachmentState colorBlendAttachmentStateInfo[1] = { };
	colorBlendAttachmentStateInfo[0].colorWriteMask = 0xf;
	colorBlendAttachmentStateInfo[0].blendEnable = VK_FALSE;
	colorBlendAttachmentStateInfo[0].alphaBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentStateInfo[0].colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachmentStateInfo[0].srcColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentStateInfo[0].dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentStateInfo[0].srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachmentStateInfo[0].dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

	//颜色融混状态的部分
	VkPipelineColorBlendStateCreateInfo colorBlendStateInfo = { };
	colorBlendStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendStateInfo.flags = 0;
	colorBlendStateInfo.pNext = NULL;
	colorBlendStateInfo.attachmentCount = 1;
	colorBlendStateInfo.pAttachments = colorBlendAttachmentStateInfo;
	colorBlendStateInfo.logicOpEnable = VK_FALSE;
	colorBlendStateInfo.blendConstants[0] = 1.0f;
	colorBlendStateInfo.blendConstants[1] = 1.0f;
	colorBlendStateInfo.blendConstants[2] = 1.0f;
	colorBlendStateInfo.blendConstants[3] = 1.0f;

	//视口状态的部分
	VkPipelineViewportStateCreateInfo viewportStateInfo = { };
	viewportStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateInfo.pNext = NULL;
	viewportStateInfo.flags = 0;
	viewportStateInfo.viewportCount = NUMBER_OF_VIEWPORTS;
	viewportStateInfo.scissorCount = NUMBER_OF_SCISSORS;
	viewportStateInfo.pScissors = NULL;
	viewportStateInfo.pViewports = NULL;

	//设置动态状态的数量和VkDynamicState枚举量
	//这样流水线就会使用动态状态指令中设置的值来替换流水线创建状态中默认的值
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStateEnables[dynamicState.dynamicStateCount++] = VK_DYNAMIC_STATE_SCISSOR;

	//深度/模板状态的部分
	VkPipelineDepthStencilStateCreateInfo depthStencilStateInfo = { };
	depthStencilStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateInfo.pNext = NULL;
	depthStencilStateInfo.flags = 0;
	depthStencilStateInfo.depthTestEnable = includeDepth;
	depthStencilStateInfo.depthWriteEnable = includeDepth;
	depthStencilStateInfo.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencilStateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateInfo.back.failOp = VK_STENCIL_OP_KEEP;
	depthStencilStateInfo.back.passOp = VK_STENCIL_OP_KEEP;
	depthStencilStateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;
	depthStencilStateInfo.back.compareMask = 0;
	depthStencilStateInfo.back.reference = 0;
	depthStencilStateInfo.back.depthFailOp = VK_STENCIL_OP_KEEP;
	depthStencilStateInfo.back.writeMask = 0;
	depthStencilStateInfo.minDepthBounds = 0;
	depthStencilStateInfo.maxDepthBounds = 0;
	depthStencilStateInfo.stencilTestEnable = VK_FALSE;
	depthStencilStateInfo.front = depthStencilStateInfo.back;

	//多重采样状态的部分
	VkPipelineMultisampleStateCreateInfo   multiSampleStateInfo = { };
	multiSampleStateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multiSampleStateInfo.pNext = NULL;
	multiSampleStateInfo.flags = 0;
	multiSampleStateInfo.pSampleMask = NULL;
	multiSampleStateInfo.rasterizationSamples = NUM_SAMPLES;
	multiSampleStateInfo.sampleShadingEnable = VK_FALSE;
	multiSampleStateInfo.alphaToCoverageEnable = VK_FALSE;
	multiSampleStateInfo.alphaToOneEnable = VK_FALSE;
	multiSampleStateInfo.minSampleShading = 0.0;

	//通过一个VkGraphicsPipelineCreateInfo结构体来设置可编程阶段,固定函数流水线阶段的渲染通道,子通道,以及流水线布局
	VkGraphicsPipelineCreateInfo pipelineInfo = { };
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pNext = NULL;
	pipelineInfo.layout = drawableObj->pipelineLayout;
	pipelineInfo.basePipelineHandle = 0;
	pipelineInfo.basePipelineIndex = 0;
	pipelineInfo.flags = 0;
	pipelineInfo.pVertexInputState = &vertexInputStateInfo;
	pipelineInfo.pInputAssemblyState = &inputAssemblyInfo;
	pipelineInfo.pRasterizationState = &rasterStateInfo;
	pipelineInfo.pColorBlendState = &colorBlendStateInfo;
	pipelineInfo.pTessellationState = NULL;
	pipelineInfo.pMultisampleState = &multiSampleStateInfo;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.pViewportState = &viewportStateInfo;
	pipelineInfo.pDepthStencilState = &depthStencilStateInfo;
	pipelineInfo.pStages = shaderObj->shaderStages;
	pipelineInfo.stageCount = 2;
	pipelineInfo.renderPass = appObj->rendererObj->renderPass;
	pipelineInfo.subpass = 0;

	//创建图形流水线
	if(vkCreateGraphicsPipelines(deviceObj->device, pipelineCache, 1, &pipelineInfo, NULL, pipeline) == VK_SUCCESS)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//销毁流水线
void VulkanPipeline::destroyPipelineCache()
{
	vkDestroyPipelineCache(deviceObj->device, pipelineCache, NULL);
}
