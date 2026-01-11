/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_PROFILER_HPP__
#define __SLRD_PROFILER_HPP__

#include <cstdint>
namespace slrd {
    class APIProfilerData;

    struct ProfilerData {
        uint64_t allocatedObjects;
        uint64_t allocations;

        uint64_t memoryUsed;
    };

    class Profiler {
        const APIProfilerData& getAPIData ();
    };
};

#endif /* #define __SLRD_PROFILER_HPP__ */
