/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_COMMAND_QUEUE_HPP__
#define __SLRD_VULKAN_COMMAND_QUEUE_HPP__

#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <slrd/commandqueue.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;
    class VKCommandBuffer;

    SLRD_RESOURCE_DEFINE_TYPE(VKCommandQueue);
    class VKCommandQueue : 
            public VKDeviceObject<ICommandQueue>,
            public VKResource<VKCommandQueue> {
    private:
        VkQueue m_queue = VK_NULL_HANDLE;
        VkCommandPool m_pool = VK_NULL_HANDLE;

        uint32_t m_queueFamily;

        /* The buffers allocated from this queue */
        std::vector<Ref<VKCommandBuffer>> m_buffers;

    public:
        VKCommandQueue () = default;
        ~VKCommandQueue ();

        [[nodiscard]] VkQueue getCommandQueue () const {
            return m_queue;
        }

        [[nodiscard]] uint32_t getCommandQueueFamily () const {
            return m_queueFamily;
        }

        [[nodiscard]] auto& getDevice () {
            return m_device;
        }

        [[nodiscard]] VkCommandPool getCommandPool () const {
            return m_pool;
        }

        int init (VKDevice *device, const CommandQueueInfo& info);
        ICommandBuffer *getCommandBuffer (bool primary) final override;

        int reset () final override;
        int wait () final override;

        int submit (const SubmitInfo& info) final override;
    };

    inline VKCommandQueue *createVKCommandQueue (
            VKDevice *device,
            const CommandQueueInfo& info) {
        return makeResource<VKCommandQueue> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_COMMAND_QUEUE_HPP__ */
