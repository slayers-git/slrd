/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_TEXTURE_HPP__
#define __SLRD_TEXTURE_HPP__

#include "format.hpp"
#include <cstdint>
#include <memory>

namespace slrd {
    enum TextureType {
        TEXTURE_TYPE_1D,
        TEXTURE_TYPE_2D,
        TEXTURE_TYPE_3D,

        /* Cubemap type is only used in creation of texture view */
        TEXTURE_TYPE_CUBE_MAP
    };

    enum TextureTilingMode {
        TEXTURE_TILING_OPTIMAL,
        TEXTURE_TILING_LINEAR,
    };

    enum TextureUsage : uint8_t {
        TEXTURE_USAGE_TRANSFER_SRC = (1 << 0),
        TEXTURE_USAGE_TRANSFER_DST = (1 << 1),
        TEXTURE_USAGE_SAMPLED = (1 << 2),
        TEXTURE_USAGE_COLOR_ATTACHMENT = (1 << 3),
        TEXTURE_USAGE_DEPTH = (1 << 4),
        TEXTURE_USAGE_STENCIL = (1 << 5),
        TEXTURE_USAGE_STORAGE = (1 << 6)
    };

    enum TextureAspect {
        TEXTURE_ASPECT_COLOR = (1 << 0),
        TEXTURE_ASPECT_DEPTH = (1 << 1),
        TEXTURE_ASPECT_STENCIL = (1 << 2),
    };

    using TextureUsageFlags  = uint32_t;
    using TextureAspectFlags = uint32_t;
    
    /* Texture and View Info */
    struct TextureInfo {
        TextureType type = TEXTURE_TYPE_2D;

        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t arrayLayers = 1;
        uint32_t mipmaps = 1;
        MSAACount samples = slrd::MSAA_COUNT_1;

        TextureUsageFlags usage = 0;
        TextureTilingMode tiling = TEXTURE_TILING_OPTIMAL;

        Format format = slrd::FORMAT_R8_UINT;
    };

    struct TextureViewInfo {
        TextureAspectFlags aspect = TEXTURE_ASPECT_COLOR;

        uint32_t arrayLayer = 0;
        uint32_t mipLevel   = 0;

        uint32_t arrayLayers = 1;
        uint32_t mipLevels   = 1;
    };

    class ITexture;

    /* A part of texture */
    class ITextureView {
    public:
        const TextureViewInfo& getTextureViewInfo () const;
        const std::shared_ptr<ITexture>& getTexture ();
    };

    /* Texture (array of textures) in memory */
    class ITexture {
    public:
        virtual ~ITexture () = default;

        virtual bool isValid () const = 0;
        virtual int getDimensions (uint32_t& w, uint32_t& h, uint32_t& depth) = 0;

        /* Create a custom texture view */
        virtual std::shared_ptr<ITextureView> createTextureView (const TextureViewInfo& view) = 0;
    };
};

#endif /* #define __SLRD_TEXTURE_HPP__ */
