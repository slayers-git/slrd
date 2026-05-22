/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "commandpool.hpp"
#include "vulkan/device.hpp"
#include "vulkan/commandpool.hpp"
#include "vulkan/error.hpp"

namespace slrd {
    int VKCommandPool::init (VKDevice *device, const CommandPoolInfo& info) noexcept {
        SLRD_ASSERT (device != nullptr);

        VkCommandPool vkpool;

        VkCommandPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        /* TODO: For now the API only supports multi-purpose 
         * compute/transfer/graphics queue. */
        pool_info.queueFamilyIndex = device->getQueueIndices ().graphics;

        pool_info.flags = 0;
        if (info.flags & COMMAND_POOL_FLAG_INDIVIDUAL_RESET)
            pool_info.flags |= VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        if (info.flags & COMMAND_POOL_FLAG_TRANSIENT)
            pool_info.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

        VK_WRAP_RETURN_RESULT_LOGERROR (
                vkCreateCommandPool (device->getVkDevice (), &pool_info, nullptr, &vkpool),
                "Failed to create command pool");

        m_flags = info.flags;
        m_pool  = vkpool;
        m_queue = Ref<VKCommandQueue>::share (
                static_cast<VKCommandQueue *>(info.queue));

        setParentDevice (device);
        device->allocate (OBJECT_TYPE_COMMAND_POOL, 0);
        device->vkallocate (VK_OBJECT_TYPE_COMMAND_POOL, 0);

        return 0;
    }

    int VKCommandPool::reset () {
        SLRD_ASSERT (m_pool != VK_NULL_HANDLE);
        VkResult result = vkResetCommandPool (m_device->getVkDevice (), m_pool, 0);
        RETURN_LOG_ERROR_IF (result != VK_SUCCESS, -1, "Failed to reset VkCommandPool");

        commandPoolReset ();

        return 0;
    }

    Ref<ICommandBuffer> VKCommandPool::allocate (const CommandBufferInfo& info) {
        SLRD_ASSERT (m_pool != VK_NULL_HANDLE);
        auto buffer = makeResource<VKCommandBuffer> (this, info);

        return Ref<ICommandBuffer>::adopt (buffer);
    }

    VKCommandPool::~VKCommandPool () noexcept {
        if (m_pool) {
            vkDestroyCommandPool (m_device->getVkDevice (), m_pool, nullptr);
            m_device->deallocate (OBJECT_TYPE_COMMAND_POOL, 0);
            m_device->vkdeallocate (VK_OBJECT_TYPE_COMMAND_POOL, 0);
        }
    }
};
