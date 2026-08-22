/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_SURFACE_HPP__
#define __SLRD_VULKAN_SURFACE_HPP__

#include "refcnt.hpp"
#include "slrd/surface.hpp"
#include "slrd/util/result.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    SLRD_RESOURCE_DEFINE_TYPE(VKSurface, VK_OBJECT_TYPE_SURFACE_KHR);
    class VKSurface :
        public SimpleRefCounted<ISurface>,
        public VKResource<VKSurface> {
    private:
        VkSurfaceKHR m_surface;    
        VkSurfaceFormatKHR m_format;

    public:
        struct SurfaceCapabilities {
            std::vector<VkSurfaceFormatKHR> formats;
            std::vector<VkPresentModeKHR>   presentModes;
            VkSurfaceCapabilitiesKHR capabilities;

            bool presentModeAvailable (VkPresentModeKHR mode) const;
            bool formatAvailable (const VkSurfaceFormatKHR& format);

            /* It's more likely for the platform to support VSync than it is
             * for it to not support it. */
            VkPresentModeKHR selectBestModeAvailable (bool demandVSync = true) const;
            VkSurfaceFormatKHR selectBestFormatAvailable (bool srgb) const;
        };

        ~VKSurface ();

        [[nodiscard]] VkSurfaceKHR getSurface () const {
            return m_surface;
        }

        [[nodiscard]] VkSurfaceFormatKHR getSurfaceFormat () const {
            return m_format;
        }

        int init (const SurfaceInfo& info);
        slrd::Result<SurfaceCapabilities, VkResult> queryCapabilities (VKDevice *device) const;
    };

    inline VKSurface *createVKSurface (const SurfaceInfo& info) {
        return makeResource<VKSurface> (info);
    }
};

#endif /* #define __SLRD_VULKAN_SURFACE_HPP__ */
