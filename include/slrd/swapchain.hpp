/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_SWAPCHAIN_HPP__
#define __SLRD_SWAPCHAIN_HPP__

#include "surface.hpp"
#include "texture.hpp"
#include <cstdint>
#include <memory>

namespace slrd {
    struct SwapchainInfo {
        uint32_t requestedImages = 2;
        std::shared_ptr<ISurface> surface;

        bool requireVSync = true;

        uint32_t width{};
        uint32_t height{};
    };

    struct PresentInfo {
        uint32_t image;
    };

    /* results that the present function may return */
    enum SwapchainResult {
        SWAPCHAIN_RESULT_SUCCESS = 0,
        SWAPCHAIN_RESULT_NEEDS_RESIZE = -1,
        SWAPCHAIN_RESULT_OTHER = -2
    };

    class ISwapchain {
    public:
        virtual ~ISwapchain () = default;

        virtual int acquireNextImage (uint32_t *next) = 0;
        virtual std::shared_ptr<ITextureView>& getTextureView (uint32_t id) = 0;

        virtual std::shared_ptr<ITexture>& getTexture (uint32_t id) = 0;

        virtual SwapchainResult present (const PresentInfo& info) = 0;
        /* recreate the swapchain */
        virtual int resize (uint32_t width, uint32_t height) = 0;

        /* Get format for the swapchain */
        virtual slrd::Format getFormat () const = 0;
    };
};

#endif /* #define __SLRD_SWAPCHAIN_HPP__ */
