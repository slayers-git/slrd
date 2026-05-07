/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_FENCE_HPP__
#define __SLRD_VULKAN_FENCE_HPP__

#include "slrd/fence.hpp"
#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    class VKFence : public VKDeviceObject<IFence> {
    private:
        VkFence m_fence = VK_NULL_HANDLE;

    public:
        VKFence () {}
        ~VKFence ();

        [[nodiscard]] VkFence getFence () const {
            return m_fence;
        }

        int init (VKDevice *device, bool signalled);

        int wait (uint64_t timeout) final override;
        void reset () final override;
    };

    inline VKFence *createVKFence (VKDevice *device, bool signalled) {
        return makeResource<VKFence> (device, signalled);
    }
};

#endif /* #define __SLRD_VULKAN_FENCE_HPP__ */
