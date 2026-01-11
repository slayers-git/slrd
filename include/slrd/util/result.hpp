/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_UTIL_RESULT_HPP__
#define __SLRD_UTIL_RESULT_HPP__

#include <cassert>
#include <algorithm>
#include <variant>

namespace slrd {
    template<typename T, typename E>
    class Result {
        std::variant<T, E> m_value;

    public:
        Result () noexcept = delete;

        Result (T&& value) noexcept :
            m_value (std::move (value)) {}

        Result (E&& error) noexcept :
            m_value (std::move (error)) {}

        operator bool () const noexcept {
            return hasValue ();
        }

        bool hasValue () const noexcept {
            return std::holds_alternative<T> (m_value);
        }

        T& unwrap () const noexcept {
            assert (hasValue ());
            return std::get<T> (m_value);
        }

        operator T& () const noexcept {
            return unwrap ();
        }

        const E& error () noexcept {
            assert (!hasValue ());
            return std::get<E> (m_value);
        }

        T* operator ->() noexcept {
            assert (hasValue ());
            return &std::get<T> (m_value);
        }
    };
};

#endif /* #define __SLRD_UTIL_RESULT_HPP__ */
