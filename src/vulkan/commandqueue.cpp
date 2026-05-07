/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "commandqueue.hpp"

#include "debug.hpp"
#include "device.hpp"
#include "vulkan/commandbuffer.hpp"
#include "vulkan/error.hpp"
#include "commandbuffer.hpp"
#include "swapchain.hpp"
#include "fence.hpp"
#include <set>

namespace slrd {
    int VKCommandQueue::init (VKDevice *device, const CommandQueueInfo& info) {
        SLRD_ASSERT (device != nullptr);

        VkCommandPool vkpool;

        VkCommandPoolCreateInfo pinfo {};
        pinfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        /* TODO: For now the API only supports multi-purpose 
         * compute/transfer/graphics queue. */
        pinfo.queueFamilyIndex = device->getQueueIndices ().graphics;

        pinfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        m_queueFamily = device->getQueueIndices ().graphics;

        VK_WRAP_RETURN_RESULT_LOGERROR (
                vkCreateCommandPool (device->getVkDevice (), &pinfo, nullptr, &vkpool),
                "Failed to create command pool");

        m_queue = device->getGraphicsQueue ();
        m_pool  = vkpool;
        setParentDevice (device);

        return 0;
    }

    ICommandBuffer *VKCommandQueue::getCommandBuffer (bool primary) {
        SLRD_ASSERT (m_pool != VK_NULL_HANDLE);
        auto buffer = makeResource<VKCommandBuffer> (this, primary);
        if (buffer) {
            m_buffers.push_back (Ref<VKCommandBuffer>::adopt (buffer));
        }

        return buffer;
    }

    VKCommandQueue::~VKCommandQueue () {
        wait ();
        m_buffers.clear ();

        if (m_pool) {
            vkDestroyCommandPool (m_device->getVkDevice (), m_pool, nullptr);
        }
    }

    int VKCommandQueue::reset () {
        SLRD_ASSERT (m_pool != VK_NULL_HANDLE);
        vkResetCommandPool (m_device->getVkDevice (), m_pool, 0);

        return -1;
    }
    int VKCommandQueue::wait () {
        SLRD_ASSERT (m_queue != VK_NULL_HANDLE);
        vkQueueWaitIdle (m_queue);

        return -1;
    }

    int VKCommandQueue::submit (const SubmitInfo& info) {
        SLRD_ASSERT (m_queue != VK_NULL_HANDLE);

        /* Swapchains to signal to */
        std::set<VKSwapchain *> swapchains;

        std::vector<VkCommandBuffer> vkbuffers (info.commandBuffers.size ());
        for (unsigned i = 0; i < info.commandBuffers.size (); ++i) {
            auto *cmdbuffer = static_cast<VKCommandBuffer *>(info.commandBuffers[i]);

            /* Check that this command buffer was created by the same queue */
            SLRD_ASSERT (cmdbuffer->getCommandQueue ().get () == this);
            SLRD_ASSERT (cmdbuffer != nullptr);
            
            vkbuffers[i] = cmdbuffer->getCommandBuffer ();
            swapchains.insert (
                    cmdbuffer->getSwapchainsToSingal ().begin (),
                    cmdbuffer->getSwapchainsToSingal ().end ());
        }

        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<VkPipelineStageFlags> waitStages (swapchains.size (),
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);
        waitSemaphores.reserve (swapchains.size ());
        signalSemaphores.reserve (swapchains.size ());

        for (auto& sc : swapchains) {
            /* Mark that we should wait for the rendering to be finished */
            sc->markForRender ();
            const auto& semaphores = sc->getSemaphores ();

            waitSemaphores.push_back (semaphores.first);
            signalSemaphores.push_back (semaphores.second);
        }

        VkSubmitInfo queueInfo {};
        queueInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        queueInfo.pCommandBuffers = vkbuffers.data ();
        queueInfo.commandBufferCount = vkbuffers.size ();
        queueInfo.waitSemaphoreCount = waitSemaphores.size ();
        queueInfo.pWaitSemaphores = waitSemaphores.data ();
        queueInfo.signalSemaphoreCount = signalSemaphores.size ();
        queueInfo.pSignalSemaphores = signalSemaphores.data ();
        queueInfo.pWaitDstStageMask = waitStages.data ();

        VkFence fence = VK_NULL_HANDLE;
        if (info.fence) {
            fence = static_cast<VKFence *> (info.fence)->getFence ();
        }

        /* FIXME: Queue is always graphics. */
        VK_WRAP_RETURN (
                vkQueueSubmit (m_device->getGraphicsQueue (), 1, &queueInfo, fence),
                -1);

        return 0;
    }
};
