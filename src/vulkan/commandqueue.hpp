/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_COMMAND_QUEUE_HPP__
#define __SLRD_VULKAN_COMMAND_QUEUE_HPP__

#include <rocket/rocket.hpp>
#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <slrd/commandqueue.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;
    class VKCommandBuffer;

    SLRD_RESOURCE_DEFINE_TYPE(VKCommandQueue, VK_OBJECT_TYPE_COMMAND_POOL);
    class VKCommandQueue : 
            public VKDeviceObject<ICommandQueue>,
            public VKResource<VKCommandQueue> {
    private:
        VkQueue m_queue = VK_NULL_HANDLE;
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

        int init (VKDevice *device, const CommandQueueInfo& info);

        int wait () final override;

        int submit (const SubmitInfo& info) final override;

        VkQueue handle () const {
            return m_queue;
        }
    };

    inline VKCommandQueue *createVKCommandQueue (
            VKDevice *device,
            const CommandQueueInfo& info) {
        return makeResource<VKCommandQueue> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_COMMAND_QUEUE_HPP__ */
