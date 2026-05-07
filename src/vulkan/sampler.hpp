/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_SAMPLER_HPP__
#define __SLRD_VULKAN_SAMPLER_HPP__

#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include <slrd/sampler.hpp>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    class VKSampler : public VKDeviceObject<ISampler> {
    private:
        VkSampler m_sampler = VK_NULL_HANDLE;
        
    public:
        VKSampler () {}
        ~VKSampler ();

        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] VkSampler getSampler () const {
            return m_sampler;
        }

        int init (VKDevice *device, const SamplerInfo& info);
    };

    inline VKSampler *createVKSampler (VKDevice *device, const SamplerInfo& info) {
        return makeResource<VKSampler> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_SAMPLER_HPP__ */
