/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_FORMAT_HPP__
#define __SLRD_FORMAT_HPP__

#include <cstdint>

namespace slrd {
    enum Format {
        FORMAT_UNDEFINED,

        FORMAT_BGRA8_UNORM,
        FORMAT_RGBA8_UNORM,
        FORMAT_RGBA16_UNORM,
        FORMAT_RGBA16_SNORM,
        FORMAT_RGBA8_UINT,
        FORMAT_RGBA16_SFLOAT,
        FORMAT_RGBA32_SFLOAT,

        FORMAT_R8_UINT,
        FORMAT_R16_FLOAT,
        FORMAT_R32_UINT,
        FORMAT_R32_FLOAT,

        FORMAT_RGB32_SFLOAT,
        FORMAT_RG32_SFLOAT,
        FORMAT_RGB32_UINT,
        FORMAT_RG32_UINT,
        FORMAT_RGB32_SINT,
        FORMAT_RG32_SINT,

        FORMAT_RGB8_UNORM,
        FORMAT_RGB8_SINT,
        FORMAT_RGB8_UINT,

		FORMAT_D32SFLOAT,
		FORMAT_D24UNORMS8UINT,
    };

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

    using StageFlags = uint32_t;
};

#endif /* #define __SLRD_FORMAT_HPP__ */
