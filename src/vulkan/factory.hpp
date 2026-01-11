/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_FACTORY_HPP__
#define __SLRD_VULKAN_FACTORY_HPP__

#include <memory>
namespace slrd {
    template<typename T, typename... Args>
    std::shared_ptr<T> makeResource (Args&&... args) {
        std::shared_ptr<T> x = std::make_shared<T>();
        auto res = x->init (std::forward<Args>(args)...);

        return !res ? x : nullptr;
    }
};

#endif /* #define __SLRD_VULKAN_FACTORY_HPP__ */
