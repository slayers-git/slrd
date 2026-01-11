/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "vulkan/api.hpp"
#include "vulkan/error.hpp"
#include <slrd/api.hpp>

#include <slrd/config.hpp>

namespace slrd {
    static API s_currentAPI = API_NONE;

    consteval uint8_t getSupportedApis () {
        uint8_t apis = API_NONE;

#if SLRD_VULKAN_ENABLED
        apis |= API_VULKAN;
#endif

        return apis;
    }

    /* get supported APIs */
    uint8_t querySupportedAPIs () {
        static const auto supportedApis = getSupportedApis ();

        return supportedApis;
    }

    /* initialize the library for the given API */
    int init (API api, const APIConfig& config) {
        if (s_currentAPI != API_NONE) {
            setError ("Can't initialize API, since another is already initialized");
            return -1;
        }

        int res;
        switch (api) {
#if SLRD_VULKAN_ENABLED
            case API_VULKAN:
                res = initVulkanAPI (config);
                break;
#endif
            default:
                setError ("API NOT IMPLEMENTED");
                return -1;
        }

        if (res)
            return res;

        s_currentAPI = api;
        return 0;
    }

    void deinit () {
        switch (s_currentAPI) {
#if SLRD_VULKAN_ENABLED
            case API_VULKAN:
                deinitVulkanAPI ();
                return;
#endif
            default:
                return;
        }
    }

    /* Current initialized API 
     *
     * If not initialized should return API_NONE */
    API getCurrentAPI () {
        return s_currentAPI;
    }
};
