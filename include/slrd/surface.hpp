/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_SURFACE_HPP__
#define __SLRD_SURFACE_HPP__

#include <memory>

namespace slrd {
    struct SurfaceInfo {
        union {
            void *ptr;
        } apiData;
    };

    /* Window surface to which swapchain binds */
    class ISurface {
    public:
        virtual ~ISurface () = default;
    };

    std::shared_ptr<ISurface> createSurface (const SurfaceInfo&);
};

#endif /* #define __SLRD_SURFACE_HPP__ */
