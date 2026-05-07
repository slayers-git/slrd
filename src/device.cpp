/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "vulkan/device.hpp"
#include "slrd/api.hpp"
#include "vulkan/error.hpp"
#include <slrd/device.hpp>

namespace slrd {
    Ref<IDevice> createDevice (const DeviceConfig& config) {
        API api;
        
        if ((api = slrd::getCurrentAPI ()) == slrd::API_NONE) {
            setError ("createDevice: no API initialized.");
            return nullptr;
        }

        switch (api) {
            case API_VULKAN:
                return Ref<IDevice>::adopt (createVKDevice (config));
            default:
                setError ("API NOT IMPLEMENTED");
                return nullptr;
        }
    }
}
