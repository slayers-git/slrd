/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_FACTORY_HPP__
#define __SLRD_VULKAN_FACTORY_HPP__

#include <slrd/ref.hpp>
#include <utility>

namespace slrd {
    template<typename T, typename... Args>
    T *makeResource (Args&&... args) {
        T *x = new T;
        auto res = x->init (std::forward<Args>(args)...);

        return !res ? x : nullptr;
    }
};

#endif /* #define __SLRD_VULKAN_FACTORY_HPP__ */
