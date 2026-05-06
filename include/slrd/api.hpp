/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_API_HPP__
#define __SLRD_API_HPP__

#include <cstdint>
#include <span>
#include <string>

namespace slrd {
    struct APIVersion {
        uint32_t major;
        uint32_t minor;
        uint32_t patch;

        APIVersion () : major(), minor(), patch() {}
        APIVersion (uint32_t major, uint32_t minor, uint32_t patch) :
            major(major), minor(minor), patch(patch) {}
    };

    struct APIConfig {
        std::string app_name;
        std::string dev_name;
        std::string engine_name;
        APIVersion  app_version;
        APIVersion  engine_version;
        APIVersion  api_version;

        bool debug = false;

        std::span<const char *> instance_extensions;
        std::span<const char *> instance_layers;
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
};

#endif /* #define __SLRD_API_HPP__ */
