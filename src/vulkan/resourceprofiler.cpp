#include "debug.hpp"
#include <slrd/platform/vulkan.hpp>

#include "device.hpp"

namespace slrd::platform::vulkan {
    VKResourceProfiler::VKResourceProfiler () noexcept :
        m_resources{} { }

    const ResourceUsage& VKResourceProfiler::query (VkObjectType resource_type) const noexcept {
        SLRD_ASSERT (resource_type < MAX_RESOURCE_TYPES);
        return m_resources[resource_type];
    }

    void VKResourceProfiler::allocate (VkObjectType type, VkDeviceSize size) noexcept {
        SLRD_ASSERT (type < MAX_RESOURCE_TYPES);

        std::lock_guard lock(m_mtx);

        auto& res = m_resources[type];
        res.memoryUsed += size;
        res.allocations++;
        res.allocatedObjects++;
    }

    void VKResourceProfiler::deallocate (VkObjectType type, VkDeviceSize size) noexcept {
        SLRD_ASSERT (type < MAX_RESOURCE_TYPES);

        std::lock_guard lock(m_mtx);

        auto& res = m_resources[type];
        SLRD_ASSERT (res.memoryUsed >= size);
        SLRD_ASSERT (res.allocatedObjects > 0);

        res.memoryUsed -= size;
        res.allocatedObjects--;
    }
};
