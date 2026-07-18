/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "fence.hpp"
#include "vulkan/error.hpp"

#include "device.hpp"
#include <vulkan/vulkan_core.h>

namespace slrd {
    int VKFence::init (VKDevice *device, const FenceInfo& info) {
        VkSemaphore semaphore;

        VkSemaphoreTypeCreateInfo tl_sem{};
        tl_sem.sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO;
        tl_sem.initialValue = info.initialValue;
        tl_sem.semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE;

        VkSemaphoreCreateInfo sem_info{};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        sem_info.pNext = &tl_sem;

        VK_WRAP_RETURN_LOGERROR (
                vkCreateSemaphore (device->getVkDevice (), &sem_info, nullptr, &semaphore),
                -1,
                "Failed to create VkFence");

        setParentDevice (device);
        m_semaphore = semaphore;

        device->allocate (OBJECT_TYPE_FENCE, 0);
        device->vkallocate (VK_OBJECT_TYPE_SEMAPHORE, 0);

        return 0;
    }

    int VKFence::signal (uint64_t value) {
        SLRD_ASSERT (m_semaphore);

        VkSemaphoreSignalInfo info{};
        info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO;
        info.semaphore = m_semaphore;
        info.value = value;

        VK_WRAP_RETURN_LOGERROR (vkSignalSemaphore(m_device->getVkDevice(), &info),
                -1,
                "Failed to signal the VkSemaphore");

        return 0;
    }

    int VKFence::wait (uint64_t value, uint64_t timeout) {
        SLRD_ASSERT (m_semaphore);

        VkSemaphoreWaitInfo wait_info{};
        wait_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO;
        wait_info.pSemaphores = &m_semaphore;
        wait_info.semaphoreCount = 1;
        wait_info.pValues = &value;
        wait_info.semaphoreCount = 1;

        VkResult result = vkWaitSemaphores(m_device->getVkDevice(), &wait_info, timeout);
        switch (result) {
            case VK_SUCCESS:
                return 0;
            case VK_TIMEOUT:
                return 1;
            default:
                return -1;
        }
    }

    uint64_t VKFence::getValue () const {
        SLRD_ASSERT (m_semaphore);

        uint64_t value;
        VkResult result = vkGetSemaphoreCounterValue(m_device->getVkDevice(), m_semaphore, &value);

        return result == VK_SUCCESS ? value : UINT64_MAX;
    }

    VKFence::~VKFence () {
        if (m_semaphore != VK_NULL_HANDLE) {
            vkDestroySemaphore (m_device->getVkDevice(), m_semaphore, nullptr);

            m_device->deallocate (OBJECT_TYPE_FENCE, 0);
            m_device->vkdeallocate (VK_OBJECT_TYPE_SEMAPHORE, 0);
        }
    }
};
