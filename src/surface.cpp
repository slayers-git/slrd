/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "vulkan/surface.hpp"
#include "slrd/api.hpp"
#include "vulkan/error.hpp"
#include <slrd/surface.hpp>

namespace slrd {
    std::shared_ptr<ISurface> createSurface (const SurfaceInfo& info) {
        RETURN_LOG_ERROR_IF (getCurrentAPI () == API_NONE,
                nullptr,
                "No API is initialized");

        switch (getCurrentAPI ()) {
            case API_VULKAN:
                return slrd::createVKSurface (info);
            default:
                setError ("NO IMPLEMENT");
                return nullptr;
        }
    }
};
