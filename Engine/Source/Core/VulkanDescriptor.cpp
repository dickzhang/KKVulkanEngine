#include "VulkanDescriptor.h"
#include "VulkanApplication.h"
#include "VulkanDevice.h"

VulkanDescriptor::VulkanDescriptor()
{
	deviceObj = VulkanApplication::GetInstance()->deviceObj;
}

VulkanDescriptor::~VulkanDescriptor()
{
}

void VulkanDescriptor::createDescriptor(bool useTexture)
{
	createDescriptorResources();
	createDescriptorPool(useTexture);
	createDescriptorSet(useTexture);
}

void VulkanDescriptor::destroyDescriptor()
{
	destroyDescriptorLayout();
	destroyPipelineLayouts();
	destroyDescriptorSet();
	destroyDescriptorPool();
}

void VulkanDescriptor::destroyDescriptorLayout()
{
	for(int i = 0; i < descLayout.size(); i++)
	{
		vkDestroyDescriptorSetLayout(deviceObj->device, descLayout[i], NULL);
	}
	descLayout.clear();
}

void VulkanDescriptor::destroyPipelineLayouts()
{
	vkDestroyPipelineLayout(deviceObj->device, pipelineLayout, NULL);
}

void VulkanDescriptor::destroyDescriptorPool()
{
	vkDestroyDescriptorPool(deviceObj->device, descriptorPool, NULL);
}

void VulkanDescriptor::destroyDescriptorSet()
{
	vkFreeDescriptorSets(deviceObj->device, descriptorPool, ( uint32_t )descriptorSet.size(), &descriptorSet[0]);
}
