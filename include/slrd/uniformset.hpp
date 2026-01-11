/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_UNIFORM_SET_HPP__
#define __SLRD_UNIFORM_SET_HPP__

#include "slrd/pipeline.hpp"
#include <cstdint>
#include <vector>

namespace slrd {
    class ITexture;
    class ITextureView;
    class ISampler;
    class IBuffer;

    enum UniformUpdateType {
        UNIFORM_UPDATE_TYPE_BUFFER,
        UNIFORM_UPDATE_TYPE_TEXTURE,
    };

    struct UniformUpdateDataBuffer {
        IBuffer *buffer;
        DeviceSize size;
        DeviceSize offset = 0;
    };
    struct UniformUpdateDataTexture {
        ISampler *sampler;
        ITextureView *view;

        /* The layout of the texture at the time of reading by the shader that
         * uses this descriptor. */
        TextureLayout layout = slrd::TEXTURE_LAYOUT_SHADER_READ_ONLY;
    };

    struct UniformUpdateData {
        struct UniformUpdate {
            UniformUpdateType type;
            uint32_t binding;

            uint32_t arrayElement;
            BindingType bindingType;

            std::vector<UniformUpdateDataTexture> textures;
            std::vector<UniformUpdateDataBuffer>  buffers;
        };

        slrd::ProxyArray<UniformUpdate> data;
    };

    class IUniformSet {
    public:
        virtual ~IUniformSet () = default;

        /* Update the data in the uniform */
        virtual int updateUniforms (const UniformUpdateData& data) = 0;
    };
};

#endif /* #define __SLRD_UNIFORM_SET_HPP__ */
