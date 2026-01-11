/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_RESOURCE_HPP__
#define __SLRD_VULKAN_RESOURCE_HPP__

#include "debug.hpp"
#include <memory>
#include <typeinfo>
#include <unordered_map>

namespace slrd {
    template<typename T>
    struct VKType {
        static const char *getName () {
            return typeid (T).name ();
        }
    };

    struct VKResourceProfiler {
        struct VKResourceUsage {
            const char *name;
            uint64_t allocations = 0;
            uint64_t frees = 0;
        };

        std::unordered_map<size_t, VKResourceUsage> usages {};

        template<typename T>
        VKResourceUsage& get ();
    };

    inline std::unique_ptr<VKResourceProfiler> s_resourceProfiler;

#define SLRD_RESOURCE_DEFINE_TYPE(__ClassType) \
    class __ClassType;                         \
    template<>                                 \
    struct VKType<__ClassType> {               \
        static const char *getName () {        \
            return #__ClassType;               \
        }                                      \
    };

    template<typename T>
    struct VKResource {
        VKResource () {
            /*if (!s_resourceProfiler) {*/
            /*    s_resourceProfiler = std::make_unique<VKResourceProfiler> ();*/
            /*}*/
            /*SLRD_DEBUG_INFO ("Created resource {}", VKType<T>::getName ());*/
            /*if (s_resourceProfiler) {*/
            /*    s_resourceProfiler->usages[typeid(T).hash_code ()].name = VKType<T>::getName ();*/
            /*    s_resourceProfiler->usages[typeid(T).hash_code ()].allocations++;*/
            /*}*/
        }

        ~VKResource () {
            /*SLRD_DEBUG_INFO ("Destroyed resource {}", VKType<T>::getName ());*/
            /*s_resourceProfiler->usages[typeid(T).hash_code ()].name = VKType<T>::getName ();*/
            /*s_resourceProfiler->usages[typeid(T).hash_code ()].frees++;*/
            /**/
            /*for (auto&[_, stat]: s_resourceProfiler->usages) {*/
            /*    std::cout << std::format ("- Resource {}: a: {}, f: {}, c: {}\n",*/
            /*            stat.name, stat.allocations, stat.frees, stat.allocations - stat.frees);*/
            /*}*/
        }
    };
}

#endif /* #define __SLRD_VULKAN_RESOURCE_HPP__ */
