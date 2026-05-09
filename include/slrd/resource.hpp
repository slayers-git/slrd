/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_RESOURCE_HPP__
#define __SLRD_RESOURCE_HPP__

#include <optional>
#include <string>

namespace slrd {
    /**
     * Types of objects provided by the API */
    enum ObjectType {
        OBJECT_TYPE_DEVICE = 0,
        OBJECT_TYPE_BUFFER,
        OBJECT_TYPE_TEXTURE,
        OBJECT_TYPE_TEXTURE_VIEW,
        OBJECT_TYPE_SAMPLER,
        OBJECT_TYPE_SHADER,
        OBJECT_TYPE_PIPELINE,
        OBJECT_TYPE_FENCE,
        OBJECT_TYPE_RENDER_PASS,
        OBJECT_TYPE_SWAPCHAIN,
        OBJECT_TYPE_COMMAND_QUEUE,
        OBJECT_TYPE_COMMAND_BUFFER,
        OBJECT_TYPE_UNIFORM_SET,

        OBJECT_TYPE_MAX_ENUM
    };

    class INamedResource {
    public:
        [[nodiscard]]
        virtual std::string_view getName () const noexcept {
            return "";
        };
    };
};

#endif /* #define __SLRD_RESOURCE_HPP__ */
