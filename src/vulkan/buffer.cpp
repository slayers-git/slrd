/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "buffer.hpp"
#include "vulkan/device.hpp"
#include "vulkan/error.hpp"
#include <cstring>

namespace slrd {
    int VKBuffer::init (VKDevice *device, const BufferInfo& info) {
        VkBufferCreateInfo bufInfo {};

        bufInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufInfo.size = info.size;
        bufInfo.usage = getVkBufferUsageFlags (info.usage);
        bufInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocation allocation;
        VmaAllocationCreateInfo allocationCreateInfo {};
        VmaAllocationInfo allocationInfo;

        bool isSrc = false,
             isDst = false;
        if (info.properties & BUFFER_PROPERTY_TRANSFER_SRC) {
            isSrc = true;
            bufInfo.usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }
        if (info.properties & BUFFER_PROPERTY_TRANSFER_DST) {
            isDst = true;
            bufInfo.usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        RETURN_LOG_ERROR_IF (!(isDst || isSrc) && info.coherent,
                -1,
                "The memory is coherent, but no TRANSFER_* property is set");

        if (info.gpu) {
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        } else {
            allocationCreateInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
        }
        if (isSrc && !isDst) {
            allocationCreateInfo.flags = 
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        }
        if (isDst && !isSrc) {
            allocationCreateInfo.flags =
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
        }

        if (info.coherent)
            allocationCreateInfo.requiredFlags = 
                VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;

        VkBuffer buffer;
        VK_WRAP_RETURN_LOGERROR (vmaCreateBuffer (device->getVkAllocator (), &bufInfo,
                &allocationCreateInfo, &buffer, &allocation, &allocationInfo),
                -1,
                "Failed to create buffer");

        m_allocation = allocation;
        m_buffer = buffer;
        setParentDevice (device);
        m_coherent = info.coherent;
        m_size = info.size;

        if (!info.name.empty ())
            (void)setResourceName (info.name, VK_OBJECT_TYPE_BUFFER, m_buffer);

        device->allocate (OBJECT_TYPE_BUFFER, info.size);
        device->vkallocate (VK_OBJECT_TYPE_BUFFER, info.size);

        return 0;
    }

    void *VKBuffer::map () {
        if (m_mapping) {
            return m_mapping;
        }

        if (!m_coherent)
            return nullptr;

        void *memory = nullptr;
        VK_WRAP_RETURN (vmaMapMemory (m_device->getVkAllocator (), m_allocation, &memory),
                nullptr);

        m_mapping = memory;

        return memory;
    }

    void VKBuffer::unmap () {
        if (m_mapping) {
            vmaUnmapMemory (m_device->getVkAllocator (), m_allocation);
            m_mapping = nullptr;
        }
    }

    int VKBuffer::setBuffer (const void *data, DeviceSize size) {
        auto res = updateBuffer (data, size);
        unmap ();

        return res;
    }

    int VKBuffer::updateBuffer (const void *data, DeviceSize size) {
        void *mapping = map ();
        if (!mapping)
            return -1;

        if (m_coherent) {
            std::memcpy (mapping, data, size);
        } else 
            return -1;

        return 0;
    }

    std::string_view VKBuffer::getName () const noexcept {
        return getResourceName ();
    }

    VKBuffer::~VKBuffer () {
        if (m_buffer != VK_NULL_HANDLE) {
            unmap ();
            vmaDestroyBuffer (m_device->getVkAllocator (), m_buffer, m_allocation);

            m_device->deallocate (OBJECT_TYPE_BUFFER, m_size);
            m_device->vkdeallocate (VK_OBJECT_TYPE_BUFFER, m_size);
        }
    }
};
