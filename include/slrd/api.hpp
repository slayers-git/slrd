/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_API_HPP__
#define __SLRD_API_HPP__

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace slrd {
    struct APIVersion {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;

        APIVersion () : major(), minor(), patch() {}
        APIVersion (uint32_t major, uint32_t minor, uint32_t patch) :
            major(major), minor(minor), patch(patch) {}
    };

    enum APIDebugFlag {
        API_DEBUG_FLAG_NONE = 0,
        /**
         * Enable debug layers for Vulkan */
        API_DEBUG_FLAG_LAYERS = 1,
        /**
         * Enable debug names in Vulkan.
         *
         * Setting this flag is necessary to be able to set debug names to
         * Vulkan objects, unlike in OpenGL or D3D12, where this functionality
         * is part of the core, and therefore are available at any time. */
        API_DEBUG_FLAG_NAMES  = 2,
        /**
         * Enable resource usage profiling */
        API_DEBUG_RESOURCE_PROFILER = 4,
    };

    struct APIConfig {
        std::string appName;
        std::string devName;
        std::string engineName;
        APIVersion  appVersion;
        APIVersion  engineVersion;
        APIVersion  apiVersion;

        /**
         * Enable debug */
        bool debug = false;

        /**
         * Debug flags */
        uint32_t debugFlags = API_DEBUG_FLAG_NONE;

        std::vector<const char *> instanceExtensions;
        std::vector<const char *> instanceLayers;
    };

    enum API : uint8_t {
        API_NONE   = 0,
        API_OPENGL = 1,
        API_VULKAN = (1 << 1)
    };

    /* get supported APIs */
    uint8_t querySupportedAPIs ();

    /* initialize the library for the given API */
    int init (API api, const APIConfig& config);

    /* deinitialize the library */
    void deinit ();

    /* Current initialized API 
     *
     * If not initialized should return API_NONE */
    API getCurrentAPI ();

    /**
     * Get config used to initialize the currenly used API */
    const APIConfig *getAPIConfig ();
};

#endif /* #define __SLRD_API_HPP__ */
