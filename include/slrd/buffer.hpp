/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_BUFFER_HPP__
#define __SLRD_BUFFER_HPP__

#include "types.hpp"
#include <cstdint>

namespace slrd {
    enum BufferUsage {
        BUFFER_USAGE_UNIFORM_BUFFER  = 1,
        BUFFER_USAGE_VERTEX_BUFFER   = 2,
        BUFFER_USAGE_INDEX_BUFFER    = 4,
        BUFFER_USAGE_INDIRECT_BUFFER = 8,
        BUFFER_USAGE_STORAGE_BUFFER  = 16,
    };
    using BufferUsageFlags = uint32_t;

    enum BufferProperty {
        BUFFER_PROPERTY_TRANSFER_SRC = 1,
        BUFFER_PROPERTY_TRANSFER_DST = 2,
    };

    using BufferPropertyFlags = uint32_t;

    struct BufferInfo {
        BufferUsageFlags usage = BUFFER_USAGE_UNIFORM_BUFFER;
        BufferPropertyFlags properties = 0;

        /* Will memory writes and reads always be coherent? */
        bool coherent = false;

        /* Allocation on the gpu? */
        bool gpu = false;

        /* The size of the buffer */
        DeviceSize size = 0;

        /* Debug name of the resource */
        std::string_view name = "";
    };

    SLRD_DEFINE_NAMED_OBJECT (IBuffer) {
    public:
        virtual ~IBuffer () = default;

        /* Attempt to map the buffer.
         *
         * If the operation fails, return nullptr. */
        virtual void *map () = 0;
        virtual void  unmap () = 0;

        virtual int setBuffer (const void *data, DeviceSize size) = 0;
        virtual int updateBuffer (const void *data, DeviceSize size) = 0;
    };
};

#endif /* #define __SLRD_BUFFER_HPP__ */
