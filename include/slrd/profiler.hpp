/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_PROFILER_HPP__
#define __SLRD_PROFILER_HPP__

#include "resource.hpp"
#include <atomic>
#include <cstdint>

namespace slrd {
    class APIProfilerData;

    struct ResourceUsage {
        /* How many objects are alive at the moment */
        uint64_t allocatedObjects;
        /* How many allocations have happened throughout the program's life */
        uint64_t allocations;

        /* How much memory is being used right now by this resource */
        uint64_t memoryUsed;
    };

    /**
     * Class to debug/profile resource usage of the library objects */
    class ResourceProfiler {
    public:
        ResourceProfiler ();
        ~ResourceProfiler ();

        /**
         * Get resource usage data for objects defined by the slrd library */
        ResourceUsage query (ObjectType type) const noexcept;

    private:
        void allocate (ObjectType type, uint64_t size) noexcept;
        void deallocate (ObjectType type, uint64_t size) noexcept;

        mutable std::mutex m_mtx;
        ResourceUsage m_resources[OBJECT_TYPE_MAX_ENUM];

        friend class ResourceProfilerWriter;
    };

    /**
     * Class used to write to a ResourceProfiler */
    class ResourceProfilerWriter {
    private:
        ResourceProfiler& m_profiler;

    public:
        ResourceProfilerWriter (ResourceProfiler& profiler) noexcept :
                m_profiler (profiler) { }
        ~ResourceProfilerWriter () noexcept = default;

        ResourceProfilerWriter (const ResourceProfilerWriter&) = delete;
        ResourceProfilerWriter operator= (const ResourceProfilerWriter&) = delete;

        /**
         * Tell the profiler that allocation of `size` bytes happened */
        void allocate (ObjectType type, uint64_t size) noexcept {
            m_profiler.allocate (type, size);
        }

        /**
         * Tell the profiler that deallocation of `size` bytes happened */
        void deallocate (ObjectType type, uint64_t size) noexcept {
            m_profiler.deallocate (type, size);
        }
    };
};

#endif /* #define __SLRD_PROFILER_HPP__ */
