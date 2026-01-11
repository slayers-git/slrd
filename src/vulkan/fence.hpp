/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_FENCE_HPP__
#define __SLRD_VULKAN_FENCE_HPP__

#include "slrd/fence.hpp"
#include "vulkan/factory.hpp"
#include <memory>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    class VKFence : public IFence {
    private:
        std::shared_ptr<VKDevice> m_device = nullptr;
        VkFence m_fence = VK_NULL_HANDLE;

    public:
        VKFence () {}
        ~VKFence ();

        [[nodiscard]] VkFence getFence () const {
            return m_fence;
        }

        int init (std::shared_ptr<VKDevice> device, bool signalled);

        int wait (uint64_t timeout) final override;
        void reset () final override;
    };

    inline std::shared_ptr<VKFence> createVKFence (std::shared_ptr<VKDevice> device,
            bool signalled) {
        return makeResource<VKFence> (device, signalled);
    }
};

#endif /* #define __SLRD_VULKAN_FENCE_HPP__ */
