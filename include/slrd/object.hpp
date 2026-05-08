/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_OBJECT_HPP__
#define __SLRD_OBJECT_HPP__

#include "resource.hpp"

namespace slrd {
    /**
     * An interface for any dynamic object that needs reference counting */
    class IObject {
    public:
        IObject () noexcept = default;
        virtual ~IObject () noexcept = default;

        virtual void addRef () noexcept = 0;
        virtual void release () noexcept = 0;
    };
};

#define SLRD_DEFINE_NAMED_OBJECT(__Name) \
    class __Name : public IObject, INamedResource

#endif /* #define __SLRD_OBJECT_HPP__ */
