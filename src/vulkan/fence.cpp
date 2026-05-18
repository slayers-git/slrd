/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "fence.hpp"
#include "vulkan/error.hpp"

#include "device.hpp"
#include <vulkan/vulkan_core.h>

namespace slrd {
    int VKFence::init (VKDevice *device, bool signalled) {
        VkFence fence;

        VkFenceCreateInfo fInfo {};
        fInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fInfo.flags = signalled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;
        VK_WRAP_RETURN_LOGERROR (vkCreateFence (device->getVkDevice (), &fInfo, nullptr, &fence),
                -1,
                "Failed to create VkFence");

        setParentDevice (device);
        m_fence = fence;

        device->allocate (OBJECT_TYPE_FENCE, 0);
        device->vkallocate (VK_OBJECT_TYPE_FENCE, 0);

        return 0;
    }

    int VKFence::wait (uint64_t timeout) {
        VkResult result = vkWaitForFences (m_device->getVkDevice (), 1, &m_fence, VK_FALSE, timeout);
        switch (result) {
            case VK_SUCCESS:
                return 0;
            case VK_TIMEOUT:
                return 1;
            default:
                return -1;
        }
    }

    void VKFence::reset () {
        vkResetFences (m_device->getVkDevice (), 1, &m_fence);
    }

    VKFence::~VKFence () {
        if (m_fence != VK_NULL_HANDLE) {
            vkDestroyFence (m_device->getVkDevice (), m_fence, nullptr);

            m_device->deallocate (OBJECT_TYPE_FENCE, 0);
            m_device->vkdeallocate (VK_OBJECT_TYPE_FENCE, 0);
        }
    }
};
