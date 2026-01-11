/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_UTIL_PROXYARRAY_HPP__
#define __SLRD_UTIL_PROXYARRAY_HPP__

#include <cstddef>
#include <type_traits>
#include <cassert>

namespace slrd {
    template<typename T>
    class ProxyArray {
    private:
        const T *m_data;
        size_t m_size;

    public:
        using element_type = T;

        /* For STL containers */
        template<typename Container,
                 typename std::enable_if<
                     std::is_convertible<decltype(std::declval<Container>().size ()), size_t>::value && 
                     std::is_convertible<decltype (std::declval<Container>().data ()), T *>::value, bool>::type = true>
        constexpr ProxyArray (const Container& v) noexcept :
            m_data (v.data ()),
            m_size (v.size ()) {}

        constexpr ProxyArray () noexcept :
            m_data (),
            m_size () {}

        /* Move */
        constexpr ProxyArray (ProxyArray&& other) noexcept :
            m_data (other.m_data),
            m_size (other.m_size) {
            other.m_size = 0;
            other.m_data = nullptr;
        }
        
        constexpr ProxyArray (const ProxyArray& other) noexcept :
            m_data (other.m_data),
            m_size (other.m_size) { }

        constexpr ProxyArray& operator= (ProxyArray&& other) noexcept {
            if (&other != this) {
                m_data = other.m_data;
                m_size = other.m_size;
            }

            return *this;
        }

        /* Copy operator (only copies references) */
        constexpr ProxyArray& operator= (const ProxyArray& other) noexcept {
            m_data = other.m_data;
            m_size = other.m_size;

            return *this;
        }

        /* for nullptr */
        constexpr ProxyArray (std::nullptr_t) noexcept :
            m_data (),
            m_size () {}

        /* For raw pointers */
        constexpr ProxyArray (const T *array, size_t size = 1) noexcept :
            m_data (array),
            m_size (size) {}

        /* For C-style arrays */
        template<size_t Count>
        constexpr ProxyArray (const T (&array)[Count]) noexcept :
            m_data (array),
            m_size (Count) {}

        template<size_t Count>
        constexpr ProxyArray& operator= (const T (&array)[Count]) noexcept {
            m_data = array;
            m_size = Count;

            return *this;
        }

        constexpr const T *data () const noexcept {
            return m_data;
        }

        constexpr size_t size () const noexcept {
            return m_size;
        }

        constexpr const T& operator[] (size_t idx) const noexcept {
            assert (idx < m_size);
            return m_data[idx];
        }

        constexpr const T *begin () const noexcept {
            return m_data;
        }
        constexpr const T *end () const noexcept {
            return m_data + m_size;
        }

        constexpr bool empty () const noexcept {
            return !m_size;
        }
    };
};

#endif /* #define __SLRD_UTIL_PROXYARRAY_HPP__ */
