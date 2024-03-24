﻿#include "ModuleBase.h"
#include "DVKDefaultRes.h"
#include "DVKCommand.h"

void ModuleBase::Setup()
{
    auto vulkanRHI    = GetVulkanRHI();
    auto vulkanDevice = vulkanRHI->GetDevice();

    m_SwapChain     = vulkanRHI->GetSwapChain();

    m_Device        = vulkanDevice->GetInstanceHandle();
    m_VulkanDevice  = vulkanDevice;
    m_GfxQueue      = vulkanDevice->GetGraphicsQueue()->GetHandle();
    m_PresentQueue  = vulkanDevice->GetPresentQueue()->GetHandle();

    m_FrameWidth    = vulkanRHI->GetSwapChain()->GetWidth();
    m_FrameHeight   = vulkanRHI->GetSwapChain()->GetHeight();
}

int32 ModuleBase::AcquireBackbufferIndex()
{
    int32 backBufferIndex = m_SwapChain->AcquireImageIndex(&m_PresentComplete);
    return backBufferIndex;
}

void ModuleBase::Present(int backBufferIndex)
{
    VkSubmitInfo submitInfo = {};
    submitInfo.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pWaitDstStageMask    = &m_WaitStageMask;
    submitInfo.pWaitSemaphores      = &m_PresentComplete;
    submitInfo.waitSemaphoreCount   = 1;
    submitInfo.pSignalSemaphores    = &m_RenderComplete;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pCommandBuffers      = &(m_CommandBuffers[backBufferIndex]);
    submitInfo.commandBufferCount   = 1;

    vkResetFences(m_Device, 1, &(m_Fences[backBufferIndex]));

    VERIFYVULKANRESULT(vkQueueSubmit(m_GfxQueue, 1, &submitInfo, m_Fences[backBufferIndex]));
    vkWaitForFences(m_Device, 1, &(m_Fences[backBufferIndex]), true, MAX_uint64);

    // present
    m_SwapChain->Present(m_VulkanDevice->GetGraphicsQueue(), m_VulkanDevice->GetPresentQueue(), &m_RenderComplete);
}

uint32 ModuleBase::GetMemoryTypeFromProperties(uint32 typeBits, VkMemoryPropertyFlags properties)
{
    uint32 memoryTypeIndex = 0;
    GetVulkanRHI()->GetDevice()->GetMemoryManager()->GetMemoryTypeFromProperties(typeBits, properties, &memoryTypeIndex);
    return memoryTypeIndex;
}

void ModuleBase::DestroyPipelineCache()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    vkDestroyPipelineCache(device, m_PipelineCache, VULKAN_CPU_ALLOCATOR);
    m_PipelineCache = VK_NULL_HANDLE;
}

void ModuleBase::CreatePipelineCache()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

    VkPipelineCacheCreateInfo createInfo;
    ZeroVulkanStruct(createInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
    VERIFYVULKANRESULT(vkCreatePipelineCache(device, &createInfo, VULKAN_CPU_ALLOCATOR, &m_PipelineCache));
}

void ModuleBase::CreateFences()
{
    VkDevice device  = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    int32 frameCount = GetVulkanRHI()->GetSwapChain()->GetBackBufferCount();

    VkFenceCreateInfo fenceCreateInfo;
    ZeroVulkanStruct(fenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    m_Fences.resize(frameCount);
    for (int32 i = 0; i < m_Fences.size(); ++i)
    {
        VERIFYVULKANRESULT(vkCreateFence(device, &fenceCreateInfo, VULKAN_CPU_ALLOCATOR, &m_Fences[i]));
    }

    VkSemaphoreCreateInfo createInfo;
    ZeroVulkanStruct(createInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
    vkCreateSemaphore(device, &createInfo, VULKAN_CPU_ALLOCATOR, &m_RenderComplete);
}

void ModuleBase::DestroyFences()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

    for (int32 i = 0; i < m_Fences.size(); ++i)
    {
        vkDestroyFence(device, m_Fences[i], VULKAN_CPU_ALLOCATOR);
    }

    vkDestroySemaphore(device, m_RenderComplete, VULKAN_CPU_ALLOCATOR);
}

void ModuleBase::CreateDefaultRes()
{
    DVKCommandBuffer* cmdbuffer = DVKCommandBuffer::Create(GetVulkanRHI()->GetDevice(), m_CommandPool);
    DVKDefaultRes::Init(GetVulkanRHI()->GetDevice(), cmdbuffer);
    delete cmdbuffer;
}

void ModuleBase::DestroyDefaultRes()
{
    DVKDefaultRes::Destroy();
}

void ModuleBase::CreateCommandBuffers()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();

    VkCommandPoolCreateInfo cmdPoolInfo;
    ZeroVulkanStruct(cmdPoolInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    cmdPoolInfo.queueFamilyIndex = GetVulkanRHI()->GetDevice()->GetPresentQueue()->GetFamilyIndex();
    cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VERIFYVULKANRESULT(vkCreateCommandPool(device, &cmdPoolInfo, VULKAN_CPU_ALLOCATOR, &m_CommandPool));

    VkCommandPoolCreateInfo computePoolInfo;
    ZeroVulkanStruct(computePoolInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
    computePoolInfo.queueFamilyIndex = GetVulkanRHI()->GetDevice()->GetComputeQueue()->GetFamilyIndex();
    computePoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    VERIFYVULKANRESULT(vkCreateCommandPool(device, &computePoolInfo, VULKAN_CPU_ALLOCATOR, &m_ComputeCommandPool));

    VkCommandBufferAllocateInfo cmdBufferInfo;
    ZeroVulkanStruct(cmdBufferInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
    cmdBufferInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmdBufferInfo.commandBufferCount = 1;
    cmdBufferInfo.commandPool        = m_CommandPool;

    m_CommandBuffers.resize(GetVulkanRHI()->GetSwapChain()->GetBackBufferCount());
    for (int32 i = 0; i < m_CommandBuffers.size(); ++i)
    {
        vkAllocateCommandBuffers(device, &cmdBufferInfo, &(m_CommandBuffers[i]));
    }
}

void ModuleBase::DestroyCommandBuffers()
{
    VkDevice device = GetVulkanRHI()->GetDevice()->GetInstanceHandle();
    for (int32 i = 0; i < m_CommandBuffers.size(); ++i)
    {
        vkFreeCommandBuffers(device, m_CommandPool, 1, &(m_CommandBuffers[i]));
    }

    vkDestroyCommandPool(device, m_CommandPool, VULKAN_CPU_ALLOCATOR);

    vkDestroyCommandPool(device, m_ComputeCommandPool, VULKAN_CPU_ALLOCATOR);
}
