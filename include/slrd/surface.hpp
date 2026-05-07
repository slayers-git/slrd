/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_SURFACE_HPP__
#define __SLRD_SURFACE_HPP__

#include "object.hpp"
#include "slrd/ref.hpp"
#include <memory>

namespace slrd {
    struct SurfaceInfo {
        union {
            void *ptr;
        } apiData;
    };

    /* Window surface to which swapchain binds */
    class ISurface : public IObject {
    public:
        virtual ~ISurface () = default;
    };

    Ref<ISurface> createSurface (const SurfaceInfo&);
};

#endif /* #define __SLRD_SURFACE_HPP__ */
