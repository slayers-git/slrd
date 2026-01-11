/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __ARENA_HPP__
#define __ARENA_HPP__

#include <array>
#include <cstdint>

template<size_t Size, typename Char = uint8_t, typename Pointer = Char *>
class Arena {
private:
    /* The array containing the data */
    std::array<Char, Size> m_data;
    /* The pointer */
    size_t m_pointer{};

public:
    using element_type = Char;
    using size_type = size_t;
    using pointer_type = Pointer;

    constexpr Arena () = default;
    constexpr Arena (const Arena& other) {
        std::copy (m_data.begin (), other.m_data.begin (), other.m_pointer);
        m_pointer = other.m_pointer;
    }
    constexpr Arena (Arena&& other) {
        m_data = std::move (other.m_data);
        m_pointer = other.m_pointer;
    }

    constexpr Arena& operator= (const Arena& other) {
        std::copy (m_data.begin (), other.m_data.begin (), other.m_data);
        m_pointer = other.m_pointer;

        return *this;
    }
    constexpr Arena& operator= (Arena&& other) {
        if (this == &other)
            return this;

        m_data = std::move (other.m_data);
        m_pointer = other.m_pointer;
    }

    [[nodiscard]]
    constexpr size_type size () const {
        return Size;
    }

    [[nodiscard]]
    constexpr size_type used () const {
        return m_pointer;
    }

    [[nodiscard]]
    constexpr pointer_type allocate (size_type size) {
        if (!size || (m_pointer + size >= Size)) {
            return nullptr;
        }

        pointer_type allocation = &m_data[m_pointer];
        m_pointer += size;
        
        return allocation;
    }
};

#endif /* #define __ARENA_HPP__ */
