/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_REFCOUNT_HPP__
#define __SLRD_VULKAN_REFCOUNT_HPP__

#include <atomic>
#include <utility>

namespace slrd {
    /* Simple reference counter, that requires no separate allocations 
     * (unlike std::shared_ptr), requires explicit control over the
     * lifetime */
    template<typename T,
        typename CountType = std::atomic_int::value_type>
    class SimpleRefWrap {
    private:
        mutable std::atomic<CountType> m_ref;
        T m_object;

    public:
        constexpr SimpleRefWrap () : m_ref (1) {}
        constexpr ~SimpleRefWrap () {}

        constexpr SimpleRefWrap (SimpleRefWrap&& other) :
            m_ref (std::move (other.m_ref)),
            m_object (std::exchange (other.m_object)) {}
        constexpr SimpleRefWrap (const SimpleRefWrap&) = delete;

        constexpr SimpleRefWrap& operator= (const SimpleRefWrap& counter)
            = delete;
        constexpr SimpleRefWrap&& operator= (SimpleRefWrap& other) {
            if (&other != this) {
                m_ref = std::move (m_ref);
                m_object = std::exchange (m_object);
            }
        }

        const T *get () const {
            ++m_ref;
            return m_object;
        }

        T *get () {
            ++m_ref;
            return m_object;
        }

        const void put () const {
            --m_ref;
        }
    };
};

#endif /* #define __SLRD_VULKAN_REFCOUNT_HPP__ */
