/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_RESOURCE_HPP__
#define __SLRD_VULKAN_RESOURCE_HPP__

#include "debug.hpp"
#include "slrd/profiler.hpp"
#include "vulkan/api.hpp"
#include <memory>
#include <typeinfo>
#include <unordered_map>

#include <atomic>

namespace slrd {
    template<typename T>
    struct VKType {
        static const char *getName () {
            return typeid (T).name ();
        }
    };

    // struct VKResourceProfiler : public ResourceProfiler {
    //     struct VKResourceUsage {
    //         std::atomic<uint64_t> allocated;
    //         std::atomic<uint64_t> live;
    //         std::atomic<uint64_t> memory;
    //     };
    //
    //     std::unordered_map<size_t, VKResourceUsage> usages {};
    //
    //     template<typename T>
    //     VKResourceUsage& get ();
    // };
    //
#define SLRD_RESOURCE_DEFINE_TYPE(__ClassType, __VkType)   \
    class __ClassType;                           \
    template<>                                   \
    struct VKType<__ClassType> {                 \
        static const char *getName () {          \
            return #__ClassType;                 \
        }                                        \
        static const VkObjectType getVkType () { \
            return __VkType;                     \
        }                                        \
    };

    template<typename Base>
    struct VKResource {

    };

    /**
     * Mixin for named Vulkan resources */
    template<typename Base>
    struct VKNamedResource : VKResource<Base> {
        template<typename T>
        bool setResourceName (std::string_view name, VkObjectType type, T handle) noexcept {
#if defined (SLRD_REQUIRE_DEBUG_NAMES)
            SLRD_COMPLAIN_RETURN (
                    !(getAPIConfig ()->debug_flags & API_DEBUG_FLAG_NAMES),
                    false,
                    "setResourceName() is used, but API_DEBUG_FLAG_NAMES is not set");

            m_name = name;

            auto device = self ()->getDevice ();
            auto vkdevice = device->getVkDevice ();
            auto vkSetDebugUtilsObjectNameEXT = vkapi->pfns.vkSetDebugUtilsObjectNameEXT;

            VkDebugUtilsObjectNameInfoEXT info {};
            info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            info.objectType   = type;
            info.objectHandle = reinterpret_cast<uint64_t> (handle);
            info.pObjectName  = m_name.c_str ();

            VkResult result = vkSetDebugUtilsObjectNameEXT (vkdevice, &info);
            SLRD_COMPLAIN_RETURN (
                    result != VK_SUCCESS, false,
                    "Failed to set name {} for {} at {}",
                    name, VKType<Base>::getName (), (void *)(self ()->handle ()));

            return true;
#else
            return false;
#endif
        }

        std::string_view getResourceName () const noexcept {
            return m_name;
        }

    private:
        std::string m_name = "";

        Base* self() {
            return static_cast<Base *>(this);
        }
    };
}

#endif /* #define __SLRD_VULKAN_RESOURCE_HPP__ */
