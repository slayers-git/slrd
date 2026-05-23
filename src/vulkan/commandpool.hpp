/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_COMMAND_POOL_HPP__
#define __SLRD_VULKAN_COMMAND_POOL_HPP__

#include "commandqueue.hpp"
#include "vulkan/commandbuffer.hpp"
#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <slrd/commandpool.hpp>

namespace slrd {
    SLRD_RESOURCE_DEFINE_TYPE(VKCommandPool, VK_OBJECT_TYPE_COMMAND_POOL);
    class VKCommandPool :
        public VKDeviceObject<ICommandPool>,
        public VKNamedResource<VKCommandPool> {
    public:
        rocket::thread_safe_signal<void()> commandPoolReset;

    private:
        VkCommandPool m_pool = VK_NULL_HANDLE;

        uint32_t m_flags;

    public:
        VKCommandPool () noexcept = default;
        ~VKCommandPool () noexcept;

        int init (VKDevice *device, const CommandPoolInfo& info) noexcept;

        /**
         * Reset buffers that were allocated from this pool */
        int reset () override final;

        inline VkCommandPool getVkCommandPool () const noexcept {
            return m_pool;
        }

        inline uint32_t getFlags () const noexcept {
            return m_flags;
        }

        /**
         * Allocate a CommandBuffer from this pool */
        Ref<ICommandBuffer> allocate (const CommandBufferInfo&) override final;
    };

    inline VKCommandPool *createVKCommandPool (
            VKDevice *device,
            const CommandPoolInfo& info) noexcept {
        return makeResource<VKCommandPool> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_COMMAND_POOL_HPP__ */
