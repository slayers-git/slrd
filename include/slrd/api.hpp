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
        API_DEBUG_FLAG_NAMES  = 2
    };

    struct APIConfig {
        std::string app_name;
        std::string dev_name;
        std::string engine_name;
        APIVersion  app_version;
        APIVersion  engine_version;
        APIVersion  api_version;

        /**
         * Enable debug layers */
        bool debug = false;
        /**
         * Debug layer flags */
        uint32_t debug_flags = API_DEBUG_FLAG_NONE;

        std::vector<const char *> instance_extensions;
        std::vector<const char *> instance_layers;
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
