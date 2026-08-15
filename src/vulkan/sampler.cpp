/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "sampler.hpp"
#include "debug.hpp"
#include "device.hpp"
#include "error.hpp"

namespace slrd {
    int VKSampler::init (VKDevice *device,
            const SamplerInfo& info) {
        SLRD_ASSERT (device != nullptr);
        
        VkSampler vksampler;

        VkSamplerCreateInfo smpInfo {};
        smpInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        smpInfo.mipLodBias = info.mipLodBias;
        smpInfo.maxLod = info.maxLod;
        smpInfo.minLod = info.minLod;
        smpInfo.magFilter = getVkFilter (info.magFilter);
        smpInfo.minFilter = getVkFilter (info.minFilter);
        smpInfo.mipmapMode = getVkSamplerMipmapMode (info.mipmapMode);
        smpInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; 
        smpInfo.addressModeU = getVkSamplerAddressMode (info.addressModeU); 
        smpInfo.addressModeV = getVkSamplerAddressMode (info.addressModeV); 
        smpInfo.addressModeW = getVkSamplerAddressMode (info.addressModeW); 
        smpInfo.anisotropyEnable = info.anisotropy;
        smpInfo.maxAnisotropy    = info.anisotropyMax;
        
        VK_WRAP_RETURN (vkCreateSampler (device->getVkDevice (), &smpInfo, nullptr, &vksampler), -1);

        setParentDevice (device);
        m_sampler = vksampler;

        device->allocate (OBJECT_TYPE_SAMPLER, 0);
        device->vkallocate (VK_OBJECT_TYPE_SAMPLER, 0);

        if (!info.name.empty ())
            setResourceName (info.name, VK_OBJECT_TYPE_SAMPLER, m_sampler);

        return 0;
    }

    std::string_view VKSampler::getName () const noexcept {
        return getResourceName ();
    }


    VKSampler::~VKSampler () {
        if (m_sampler) {
            vkDestroySampler (m_device->getVkDevice (), m_sampler, nullptr);

            m_device->deallocate (OBJECT_TYPE_SAMPLER, 0);
            m_device->vkdeallocate (VK_OBJECT_TYPE_SAMPLER, 0);
        }
    }
};
