/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_FENCE_HPP__
#define __SLRD_VULKAN_FENCE_HPP__

#include "resource.hpp"

#include "slrd/fence.hpp"
#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    SLRD_RESOURCE_DEFINE_TYPE(VKFence, VK_OBJECT_TYPE_FENCE);
    class VKFence :
            public VKDeviceObject<IFence>,
            public VKResource<VKFence> {
    private:
        VkSemaphore m_semaphore = VK_NULL_HANDLE;

    public:
        VKFence () {}
        ~VKFence ();

        // [[nodiscard]] VkFence getFence () const {
        //     return m_fence;
        // }

        [[nodiscard]]
        VkSemaphore getSemaphore () const {
            return m_semaphore;
        }

        int init (VKDevice *device, const FenceInfo& info);

        int signal (uint64_t value) final override;
        int wait (uint64_t value, uint64_t timeout) final override;
        uint64_t getValue () const final override;

        VkSemaphore handle () const {
            return m_semaphore;
        }
    };

    inline VKFence *createVKFence (VKDevice *device, const FenceInfo& info) {
        return makeResource<VKFence> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_FENCE_HPP__ */
