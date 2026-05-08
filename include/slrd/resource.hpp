/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_RESOURCE_HPP__
#define __SLRD_RESOURCE_HPP__

#include <optional>
#include <string>

namespace slrd {
    class INamedResource {
    public:
        [[nodiscard]]
        virtual std::string_view getName () const noexcept {
            return "";
        };
    };
};

#endif /* #define __SLRD_RESOURCE_HPP__ */
