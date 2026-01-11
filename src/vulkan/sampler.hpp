/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_SAMPLER_HPP__
#define __SLRD_VULKAN_SAMPLER_HPP__

#include "vulkan/factory.hpp"
#include <memory>
#include <slrd/sampler.hpp>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;

    class VKSampler : public ISampler {
    private:
        std::shared_ptr<VKDevice> m_device = nullptr;
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

        int init (std::shared_ptr<VKDevice> device, const SamplerInfo& info);
    };

    static inline std::shared_ptr<VKSampler> createVKSampler (
            std::shared_ptr<VKDevice> device, const SamplerInfo& info) {
        return makeResource<VKSampler> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_SAMPLER_HPP__ */
