#ifndef __SLRD_REF_HPP__
#define __SLRD_REF_HPP__

#include <type_traits>

namespace slrd {
    class IObject;

    /**
     * Reference wrapper for an object derived from IObject - a intrusively
     * reference counted object */
    template<typename T>
    class Ref {
    private:
        T *m_ptr{};

        void addRef () noexcept {
            if (m_ptr)
                m_ptr->addRef ();
        }

        void release () noexcept {
            if (m_ptr)
                m_ptr->release ();
        }

    public:
        Ref () noexcept = default;

        Ref (const Ref& object) noexcept :
                m_ptr (object.m_ptr) {
            addRef ();
        }

        Ref (Ref&& other) noexcept :
                m_ptr (other.m_ptr) {
            other.m_ptr = nullptr;
        }

        Ref (std::nullptr_t) noexcept :
            m_ptr (nullptr) { };

        Ref& operator= (const Ref& other) noexcept {
            Ref copy (other);
            std::swap (m_ptr, copy.m_ptr);

            return *this;
        }

        Ref& operator= (Ref&& other) noexcept {
            std::swap (m_ptr, other.m_ptr);
            return *this;
        }

        /**
         * Create a Ref from existing pointer */
        static Ref adopt (T *ptr) noexcept {
            Ref<T> object;
            object.m_ptr = ptr;

            return object;
        }

        static Ref share (T *ptr) noexcept {
            Ref res = adopt (ptr);
            res.addRef ();

            return res;
        }

        ~Ref () noexcept {
            release ();
        }

        const T *operator-> () const noexcept {
            return m_ptr;
        }

        T *operator-> () noexcept {
            return m_ptr;
        }

        const T& operator* () const noexcept {
            return *m_ptr;
        }

        T& operator* () noexcept {
            return *m_ptr;
        }

        T *get () const noexcept {
            return m_ptr;
        }

        operator bool () const noexcept {
            return m_ptr;
        }

        bool operator== (const Ref& other) const noexcept {
            return m_ptr == other.m_ptr;
        }

        bool operator!= (const Ref& other) const noexcept {
            return m_ptr != other.m_ptr;
        }

        bool operator== (std::nullptr_t) const noexcept {
            return m_ptr == nullptr;
        }

        bool operator!= (std::nullptr_t) const noexcept {
            return m_ptr != nullptr;
        }
    };
};

#endif /* #define __SLRD_REF_HPP__ */
