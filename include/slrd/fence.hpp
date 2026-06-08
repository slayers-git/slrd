/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_FENCE_HPP__
#define __SLRD_FENCE_HPP__

#include "object.hpp"
#include <cstdint>

namespace slrd {
    class IFence : public IObject {
    public:
        virtual ~IFence () = default;

        virtual int wait (uint64_t timeout = UINT64_MAX) = 0;
        virtual void reset () = 0;

        virtual uint64_t getValue () const = 0;
    };
};

#endif /* #define __SLRD_FENCE_HPP__ */
