/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_SAMPLER_HPP__
#define __SLRD_SAMPLER_HPP__

#include "object.hpp"
#include "format.hpp"

namespace slrd {
    enum SamplerAddressMode {
        SAMPLER_ADDRESS_MODE_REPEAT = 0,
        SAMPLER_ADDRESS_MIRRORED_REPEAT = 1,
        SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE = 2,
        SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER = 3,

        SAMPLER_ADDRESS_MODE_MAX_ENUM
    };

    struct SamplerInfo {
        Filter magFilter = FILTER_LINEAR;
        Filter minFilter = FILTER_LINEAR;

        MipmapMode mipmapMode = MIPMAP_MODE_LINEAR;
        float anisotropyMax   = 1.f;
        bool  anisotropy = false;

        SamplerAddressMode addressModeU = SAMPLER_ADDRESS_MODE_REPEAT,
                           addressModeV = SAMPLER_ADDRESS_MODE_REPEAT,
                           addressModeW = SAMPLER_ADDRESS_MODE_REPEAT;

        float mipLodBias = 0.f;
        float minLod = 0.f;
        float maxLod = 1.f;

        /* Debug name of the resource */
        std::string_view name = "";
    };

    SLRD_DEFINE_NAMED_OBJECT (ISampler) {
    public:
        virtual ~ISampler () = default;
    };
};

#endif /* #define __SLRD_SAMPLER_HPP__ */
