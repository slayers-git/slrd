/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_UNIFORM_SET_HPP__
#define __SLRD_VULKAN_UNIFORM_SET_HPP__

#include "resource.hpp"

#include "vulkan/deviceobject.hpp"
#include <slrd/uniformset.hpp>

#include <vulkan/vulkan.h>

namespace slrd {
    class VKPipelineLayout;
    class DescriptorPoolManager;

    SLRD_RESOURCE_DEFINE_TYPE (VKUniformSet, VK_OBJECT_TYPE_DESCRIPTOR_SET);
    class VKUniformSet :
            public VKDeviceObject<IUniformSet>,
            public VKResource<VKUniformSet> {
    private:
        VkDevice m_device = VK_NULL_HANDLE;
        VkDescriptorSet m_set = nullptr;

        /* Manager, from which this set is allocated */
        DescriptorPoolManager *m_manager;
        /* idx into the pool from which this has been allocated */
        uint32_t m_pool;

    public:
        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] VkDescriptorSet getDescriptorSet () const {
            return m_set;
        }

        VKUniformSet () = default;
        ~VKUniformSet ();

        int init (VKPipelineLayout *pipelineLayout, uint32_t set);

        int updateUniforms (const UniformUpdateData&) final;
    };
};

#endif /* #define __SLRD_VULKAN_UNIFORM_SET_HPP__ */
