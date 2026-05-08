/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_DEVICEOBJECT_HPP__
#define __SLRD_VULKAN_DEVICEOBJECT_HPP__

#include <slrd/ref.hpp>
#include <atomic>

namespace slrd {
    class VKDevice;

    /**
     * Mixin for vulkan reference-counted objects that derive from some
     * device */
    template<typename Base>
    class VKDeviceObject : public Base {
    protected:
        /* VKDevice that is the parent of this object */
        Ref<VKDevice> m_device;

        void setParentDevice (VKDevice *device) noexcept {
            // bump up the refcnt
            m_device = Ref<VKDevice>::share (device);
        }
    private:
        mutable std::atomic<uint32_t> m_ref{1};

        Base* self() {
            return static_cast<Base *>(this);
        }

    public:
        VKDeviceObject () noexcept = default;

        VKDeviceObject (VKDeviceObject&&) noexcept = default;
        VKDeviceObject& operator= (VKDeviceObject&&) noexcept = default;

        VKDevice *getDevice () noexcept {
            return m_device.get ();
        }

        void addRef () noexcept override {
            m_ref.fetch_add (1, std::memory_order_relaxed);
        }

        void release () noexcept override {
            if (m_ref.fetch_sub (1, std::memory_order_acq_rel) == 1)
                delete this;
        }

        /**
         * Force the destruction of the object */
        void forceDestroy () noexcept {
            delete this;
        }
    };
};

#endif /* #define __SLRD_VULKAN_DEVICEOBJECT_HPP__ */
