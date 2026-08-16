/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_SAMPLER_HPP__
#define __SLRD_VULKAN_SAMPLER_HPP__

#include "vulkan/deviceobject.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <slrd/sampler.hpp>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    /* As long as values are 1:1 */
    constexpr VkFilter getVkFilter (Filter filter) {
        SLRD_ASSERT (filter < FILTER_MAX_ENUM);
        return static_cast<VkFilter>(filter);
    }

    constexpr VkSamplerMipmapMode getVkSamplerMipmapMode (MipmapMode mode) {
        SLRD_ASSERT (mode < MIPMAP_MODE_MAX_ENUM);
        return static_cast<VkSamplerMipmapMode> (mode);
    }

    constexpr VkSamplerAddressMode getVkSamplerAddressMode (SamplerAddressMode mode) {
        SLRD_ASSERT (mode < SAMPLER_ADDRESS_MODE_MAX_ENUM);
        return static_cast<VkSamplerAddressMode> (mode);
    }

    SLRD_RESOURCE_DEFINE_TYPE (VKSampler, VK_OBJECT_TYPE_SAMPLER);
    class VKSampler :
        public VKDeviceObject<ISampler>,
        public VKNamedResource<VKSampler> {
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

        VkSampler handle () const {
            return m_sampler;
        }

        std::string_view getName () const noexcept final;
    };

    inline VKSampler *createVKSampler (VKDevice *device, const SamplerInfo& info) {
        return makeResource<VKSampler> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_SAMPLER_HPP__ */
