/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_FORMAT_HPP__
#define __SLRD_FORMAT_HPP__

#include <cstdint>

namespace slrd {
    enum Format {
        FORMAT_UNDEFINED,

        FORMAT_BGRA8_UNORM,
        FORMAT_BGRA8_SRGB,

        FORMAT_RGBA8_UINT,
        FORMAT_RGBA8_SINT,
        FORMAT_RGBA8_UNORM,
        FORMAT_RGBA8_SNORM,
        FORMAT_RGBA8_SRGB,

        FORMAT_RGBA16_UINT,
        FORMAT_RGBA16_SINT,
        FORMAT_RGBA16_UNORM,
        FORMAT_RGBA16_SNORM,
        FORMAT_RGBA16_SFLOAT,

        FORMAT_RGBA32_UINT,
        FORMAT_RGBA32_SINT,
        FORMAT_RGBA32_SFLOAT,

        FORMAT_R8_UINT,
        FORMAT_R8_SINT,
        FORMAT_R8_UNORM,
        FORMAT_R8_SNORM,

        FORMAT_R16_UINT,
        FORMAT_R16_SINT,
        FORMAT_R16_UNORM,
        FORMAT_R16_SNORM,
        FORMAT_R16_SFLOAT,

        FORMAT_R32_UINT,
        FORMAT_R32_SINT,
        FORMAT_R32_SFLOAT,

        FORMAT_RG8_UINT,
        FORMAT_RG8_SINT,
        FORMAT_RG8_UNORM,
        FORMAT_RG8_SNORM,

        FORMAT_RG16_UINT,
        FORMAT_RG16_SINT,
        FORMAT_RG16_UNORM,
        FORMAT_RG16_SNORM,
        FORMAT_RG16_SFLOAT,

        FORMAT_RG32_UINT,
        FORMAT_RG32_SINT,
        FORMAT_RG32_SFLOAT,

        FORMAT_RGB32_UINT,
        FORMAT_RGB32_SINT,
        FORMAT_RGB32_SFLOAT,

        FORMAT_RGB10A2_UINT,
        FORMAT_RGB10A2_UNORM,

        FORMAT_RG11B10_UFLOAT,

        /********************/
        /* Block Compressed */
        /********************/

        FORMAT_BC1_RGB_UNORM,
        FORMAT_BC1_RGB_SRGB,
        FORMAT_BC1_RGBA_UNORM,
        FORMAT_BC1_RGBA_SRGB,

        FORMAT_BC3_UNORM,
        FORMAT_BC3_SRGB,

        FORMAT_BC4_UNORM,
        FORMAT_BC4_SNORM,

        FORMAT_BC5_UNORM,
        FORMAT_BC5_SNORM,

        FORMAT_BC6H_UFLOAT,
        FORMAT_BC6H_SFLOAT,

        FORMAT_BC7_UNORM,
        FORMAT_BC7_SRGB,

        /*********/
        /* Depth */
        /*********/

		FORMAT_D16_UNORM,
		FORMAT_D32_SFLOAT,
        FORMAT_D32_SFLOAT_S8_UINT,
        /**
         * @note In Vulkan, specifically using RADV drivers, the support for
         *       this format is not guaranteed. In slrd, if this format is not 
         *       supported, the texture created will be of D32_SFLOAT_S8_UINT
         *       format instead. */
		FORMAT_D24_UNORM_S8_UINT,
    };

    [[nodiscard]]
    constexpr bool isDepthFormat (Format format) {
        return format >= FORMAT_D16_UNORM && format <= FORMAT_D24_UNORM_S8_UINT;
    }

    [[nodiscard]]
    constexpr bool hasStencil (Format format) {
        return format == FORMAT_D32_SFLOAT_S8_UINT || format == FORMAT_D24_UNORM_S8_UINT;
    }

    enum MSAACount {
        MSAA_COUNT_1  = 1,
        MSAA_COUNT_2  = 2,
        MSAA_COUNT_4  = 4,
        MSAA_COUNT_8  = 8,
        MSAA_COUNT_16 = 16,
    };

    enum LoadOperation {
        LOAD_OPERATION_LOAD,
        LOAD_OPERATION_CLEAR,
        LOAD_OPERATION_DONT_CARE,
    };

    enum StoreOperation {
        STORE_OPERATION_STORE,
        STORE_OPERATION_DONT_CARE,
    };

    struct Viewport {
        float x{}, y{};
        float width = 800, height = 600;
        float minDepth = 0, maxDepth = 1;
    };

    struct Scissor {
        int32_t x, y;
        uint32_t w, h;
    };

	enum StageFlagBits {
		STAGE_VERTEX   = 1,
		STAGE_FRAGMENT = 1 << 1,
		STAGE_GEOMETRY = 1 << 2,
		STAGE_TESSELLATION_CONTROL = 1 << 3,
		STAGE_TESSELLATION_EVALUATION = 1 << 4,
		STAGE_COMPUTE  = 1 << 5,
	};

    constexpr uint32_t STAGE_MAX_ENUM = STAGE_COMPUTE;

    enum IndexType {
        INDEX_TYPE_UINT16,
        INDEX_TYPE_UINT32
    };

    enum Filter {
        FILTER_NEAREST = 0,
        FILTER_LINEAR  = 1,

        FILTER_MAX_ENUM
    };

    enum MipmapMode {
        MIPMAP_MODE_NEAREST = 0,
        MIPMAP_MODE_LINEAR  = 1,

        MIPMAP_MODE_MAX_ENUM,
    };

    enum TextureLayout {
        /* Derive the best layout automatically */
        TEXTURE_LAYOUT_AUTO = 0,
        /* Undefined layout */
        TEXTURE_LAYOUT_UNDEFINED = 1,
        TEXTURE_LAYOUT_GENERAL,
        TEXTURE_LAYOUT_COLOR_ATTACHMENT,
        TEXTURE_LAYOUT_SHADER_READ_ONLY,
        TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT,
        TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY,
        TEXTURE_LAYOUT_SWAPCHAIN_SRC,
        TEXTURE_LAYOUT_TRANSFER_SRC,
        TEXTURE_LAYOUT_TRANSFER_DST,
    };

    template<typename T>
    struct Rect2D {
        T x, y, w, h;
    };

    template<typename T>
    struct Rect3D {
        T x, y, z, w, h, d;
    };

    template<typename T>
    struct Offset3D {
        T x{}, y{}, z{};
    };

    using StageFlags = uint32_t;
};

#endif /* #define __SLRD_FORMAT_HPP__ */
