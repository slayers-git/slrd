#include "debug.hpp"
#include <slrd/profiler.hpp>

namespace slrd {
    ResourceProfiler::ResourceProfiler () :
            m_resources() { }

    ResourceProfiler::~ResourceProfiler () = default;

    ResourceUsage ResourceProfiler::query (ObjectType type) const noexcept {
        SLRD_ASSERT (type < OBJECT_TYPE_MAX_ENUM);
        std::lock_guard lock (m_mtx);

        return m_resources[type];
    }

    void ResourceProfiler::allocate (ObjectType type, uint64_t size) noexcept {
        SLRD_ASSERT (type < OBJECT_TYPE_MAX_ENUM);
        std::lock_guard lock (m_mtx);

        auto& res = m_resources[type];
        res.allocations++;
        res.allocatedObjects++;
        res.memoryUsed += size;
    }

    void ResourceProfiler::deallocate (ObjectType type, uint64_t size) noexcept {
        SLRD_ASSERT (type < OBJECT_TYPE_MAX_ENUM);

        std::lock_guard lock (m_mtx);

        auto& resource = m_resources[type];
        SLRD_ASSERT (resource.memoryUsed >= size);
        SLRD_ASSERT (resource.allocations >= resource.allocatedObjects);

        resource.allocatedObjects--;
        resource.memoryUsed -= size;
    }
}
