/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_REFCNT_HPP__
#define __SLRD_REFCNT_HPP__

#include <atomic>

namespace slrd {
    template<typename Base>
    class SimpleRefCounted : public Base {
    private:
        /* VKDevice that is the parent of this object */
        std::atomic<uint32_t> m_ref{1};

    public:
        void addRef () noexcept override {
            m_ref.fetch_add (1, std::memory_order_relaxed);
        }

        void release () noexcept override {
            if (m_ref.fetch_sub (1, std::memory_order_acq_rel) == 1)
                delete this;
        }
    };
};

#endif /* #define __SLRD_REFCNT_HPP__ */
