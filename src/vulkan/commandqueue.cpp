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

        m_queue = device->getGraphicsQueue ();
        m_queueFamily = device->getQueueIndices ().graphics;
        setParentDevice (device);

        device->allocate (OBJECT_TYPE_COMMAND_QUEUE, 0);

        return 0;
    }

    VKCommandQueue::~VKCommandQueue () {
        wait ();
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
            SLRD_ASSERT (cmdbuffer != nullptr);
            
            vkbuffers[i] = cmdbuffer->getCommandBuffer ();
            swapchains.insert (
                    cmdbuffer->getSwapchainsToSingal ().begin (),
                    cmdbuffer->getSwapchainsToSingal ().end ());
        }

        const auto nr_wait = info.waitFences.size() + swapchains.size();
        const auto nr_signal = info.signalFences.size() + swapchains.size();

        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkSemaphore> signalSemaphores;
        std::vector<uint64_t> waitValues (nr_wait, 0);
        std::vector<uint64_t> signalValues (nr_signal, 0);
        std::vector<VkPipelineStageFlags> waitStages (nr_wait,
                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT);

        waitSemaphores.reserve (nr_wait);
        signalSemaphores.reserve (nr_signal);

        for (int i = 0; i < info.waitFences.size(); ++i) {
            const auto& fence_data = info.waitFences[i];

            SLRD_ASSERT(fence_data.fence != nullptr);
            waitValues[i] = fence_data.value;

            VKFence *fence = static_cast<VKFence *> (fence_data.fence);
            waitSemaphores.emplace_back (fence->getSemaphore());
        }

        for (int i = 0; i < info.signalFences.size(); ++i) {
            const auto& fence_data = info.signalFences[i];

            SLRD_ASSERT(fence_data.fence != nullptr);
            signalValues[i] = fence_data.value;

            VKFence *fence = static_cast<VKFence *> (fence_data.fence);
            signalSemaphores.emplace_back (fence->getSemaphore());
        }

        for (auto& sc : swapchains) {
            /* Mark that we should wait for the rendering to be finished */
            sc->markForRender ();
            const auto& semaphores = sc->getSemaphores ();

            waitSemaphores.push_back (semaphores.first);
            signalSemaphores.push_back (semaphores.second);
        }

        VkTimelineSemaphoreSubmitInfo tl_info{};
        tl_info.sType = VK_STRUCTURE_TYPE_TIMELINE_SEMAPHORE_SUBMIT_INFO;
        tl_info.pWaitSemaphoreValues = waitValues.data();
        tl_info.waitSemaphoreValueCount = waitValues.size();
        tl_info.pSignalSemaphoreValues = signalValues.data();
        tl_info.signalSemaphoreValueCount = signalValues.size();

        VkSubmitInfo queueInfo {};
        queueInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        queueInfo.pCommandBuffers = vkbuffers.data ();
        queueInfo.commandBufferCount = vkbuffers.size ();
        queueInfo.waitSemaphoreCount = waitSemaphores.size ();
        queueInfo.pWaitSemaphores = waitSemaphores.data ();
        queueInfo.signalSemaphoreCount = signalSemaphores.size ();
        queueInfo.pSignalSemaphores = signalSemaphores.data ();
        queueInfo.pWaitDstStageMask = waitStages.data ();
        queueInfo.pNext = &tl_info;

        /* FIXME: Queue is always graphics. */
        VK_WRAP_RETURN (
                vkQueueSubmit (m_device->getGraphicsQueue (), 1, &queueInfo, nullptr),
                -1);

        return 0;
    }
};
