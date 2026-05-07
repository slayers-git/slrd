/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "surface.hpp"
#include "vulkan/api.hpp"
#include "vulkan/error.hpp"
#include "device.hpp"

#include <algorithm>

#define ARRAYLEN(__Array) \
    (sizeof (__Array) / sizeof (__Array[0]))

namespace slrd {
    int VKSurface::init (const SurfaceInfo& info) {
        RETURN_LOG_ERROR_IF (!info.apiData.ptr,
                -1,
                "VKSurface::init (): surface is nullptr!");

        m_surface = static_cast<VkSurfaceKHR>(info.apiData.ptr);
        return 0;
    }

    VKSurface::~VKSurface () {
        vkDestroySurfaceKHR (vkapi->instance, m_surface, nullptr);
    }

    slrd::Result<VKSurface::SurfaceCapabilities, VkResult> VKSurface::queryCapabilities (VKDevice *device) const {
        SurfaceCapabilities info;
        VkPhysicalDevice physicalDevice = device->getPhysicalDevice ();

        VkResult result;
        result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR (physicalDevice,
                m_surface, &info.capabilities);
        if (result != VK_SUCCESS) {
            return result;
        }

        uint32_t count;
        {
            vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, m_surface, &count,
                    nullptr);

            if (count) {
                info.formats.resize (count);
                vkGetPhysicalDeviceSurfaceFormatsKHR (physicalDevice, m_surface, &count,
                        info.formats.data ());
            }
        }

        {
            vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, m_surface, &count,
                    nullptr);

            if (count) {
                info.presentModes.resize (count);
                vkGetPhysicalDeviceSurfacePresentModesKHR (physicalDevice, m_surface, &count,
                        info.presentModes.data ());
            }
        }

        return info;
    }

    bool VKSurface::SurfaceCapabilities::presentModeAvailable (VkPresentModeKHR mode) const {
        const auto& it = std::find (presentModes.cbegin (), presentModes.cend (),
                mode);
        return it != presentModes.end ();
    }

    bool VKSurface::SurfaceCapabilities::formatAvailable (const VkSurfaceFormatKHR& format) {
        const auto& it = std::find_if (formats.begin (), formats.end (),
                [&format](const VkSurfaceFormatKHR& element) -> bool {
                    return element.format == format.format &&
                            element.colorSpace == format.colorSpace;
                });
        return it != formats.end ();
    }

    VkPresentModeKHR VKSurface::SurfaceCapabilities::selectBestModeAvailable (bool vsync) const {
        static const VkPresentModeKHR bestModes[] = {
            VK_PRESENT_MODE_MAILBOX_KHR,
            VK_PRESENT_MODE_FIFO_RELAXED_KHR,
            /* This is required to be supported by the standard */
            VK_PRESENT_MODE_FIFO_KHR,
        };

        if (!vsync && std::find (presentModes.begin (), presentModes.end (),
                    VK_PRESENT_MODE_IMMEDIATE_KHR) != presentModes.end ()) {
            return VK_PRESENT_MODE_IMMEDIATE_KHR;
        }

        for (uint32_t i = 0; i < ARRAYLEN (bestModes); ++i) {
            if (std::find (presentModes.begin (), presentModes.end (),
                        bestModes[i]) != presentModes.end ()) {
                return bestModes[i];
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkSurfaceFormatKHR VKSurface::SurfaceCapabilities::selectBestFormatAvailable () const {
        static const VkFormat bestFormats[] = {
            VK_FORMAT_B8G8R8A8_UNORM,
            VK_FORMAT_R8G8B8A8_UNORM
        };

        VkFormat format = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR color_space = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        if (formats.size () == 1 && formats[0].format == VK_FORMAT_UNDEFINED) {
            format = VK_FORMAT_B8G8R8A8_UNORM;
            /*color_space = formats[0].colorSpace;*/
        } else if (formats.size () > 0) {
            // Use one of the supported formats, prefer B8G8R8A8_UNORM.
            for (auto currentFormat : formats) {
                for (uint32_t i = 0; i < formats.size (); i++) {
                    if (currentFormat.format == bestFormats[i]) {
                        format = currentFormat.format;
                        return currentFormat;
                    }
                }
            }
        } else {
            return { VK_FORMAT_UNDEFINED, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
        }

        return { format, color_space };
    }
};
