/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_BUFFER_HPP__
#define __SLRD_VULKAN_BUFFER_HPP__

#include "deviceobject.hpp"
#include "slrd/format.hpp"
#include "vulkan/device.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <memory>
#include <slrd/buffer.hpp>

#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    inline constexpr VkBufferUsageFlags getVkBufferUsageFlags (BufferUsageFlags flags) {
        VkBufferUsageFlags vkflags = 0;

        if (flags & BUFFER_USAGE_VERTEX_BUFFER)
            vkflags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        if (flags & BUFFER_USAGE_INDEX_BUFFER)
            vkflags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        if (flags & BUFFER_USAGE_UNIFORM_BUFFER)
            vkflags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        if (flags & BUFFER_USAGE_INDIRECT_BUFFER)
            vkflags |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        if (flags & BUFFER_USAGE_STORAGE_BUFFER)
            vkflags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        return vkflags;
    }

    inline constexpr VkIndexType getVkIndexType (IndexType type) {
        VkIndexType vktype = VK_INDEX_TYPE_UINT16;

        switch (type) {
            case INDEX_TYPE_UINT16:
                vktype = VK_INDEX_TYPE_UINT16; break;
            case INDEX_TYPE_UINT32:
                vktype = VK_INDEX_TYPE_UINT32; break;
        }

        return vktype;
    }

    SLRD_RESOURCE_DEFINE_TYPE(VKBuffer, VK_OBJECT_TYPE_BUFFER);
    class VKBuffer :
            public VKDeviceObject<IBuffer>,
            public VKNamedResource<VKBuffer> {
    private:
        VkBuffer m_buffer = VK_NULL_HANDLE;
        VmaAllocation m_allocation = VK_NULL_HANDLE;

        bool m_coherent;
        VkDeviceSize m_size;

        /* persistent map */
        void *m_mapping{};

    public:
        VKBuffer () = default;
        ~VKBuffer ();

        int init (VKDevice *device, const BufferInfo& info);

        [[nodiscard]] VkBuffer getBuffer () const {
            return m_buffer;
        }

        void *map () final override;
        void unmap () final override;

        int setBuffer (const void *data, DeviceSize size) final override;
        int updateBuffer (const void *data, DeviceSize size) final override;

        std::string_view getName () const noexcept final override;

        VkBuffer handle () const {
            return m_buffer;
        }
    };

    inline VKBuffer *createVKBuffer (VKDevice *device, const BufferInfo& info) {
        return makeResource<VKBuffer> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_BUFFER_HPP__ */
