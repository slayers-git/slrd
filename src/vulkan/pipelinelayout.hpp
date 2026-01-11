/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_PIPELINE_LAYOUT_HPP__
#define __SLRD_VULKAN_PIPELINE_LAYOUT_HPP__

#include "vulkan/device.hpp"
#include <slrd/pipeline.hpp>
#include <vector>

namespace slrd {
    class VKUniformSet;

    inline constexpr VkDescriptorType getVkDescriptorType (slrd::BindingType type) {
#define __TYPE_CASE(__Type) \
        case BINDING_TYPE_ ## __Type: vktype = VK_DESCRIPTOR_TYPE_ ## __Type; break;

        VkDescriptorType vktype = VK_DESCRIPTOR_TYPE_SAMPLER;

        switch (type) {
            __TYPE_CASE (SAMPLER);
            __TYPE_CASE (UNIFORM_BUFFER);
            __TYPE_CASE (STORAGE_BUFFER);
            __TYPE_CASE (STORAGE_BUFFER_DYNAMIC);
            __TYPE_CASE (UNIFORM_BUFFER_DYNAMIC);

            case BINDING_TYPE_COMBINED_TEXTURE_SAMPLER:
                vktype = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER; break;
            case BINDING_TYPE_TEXTURE:
                vktype = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
        }

#undef __TYPE_CASE

        return vktype;
    }

    inline constexpr VkShaderStageFlags getVkShaderStageFlags (StageFlags flags) {
        VkShaderStageFlags vkflags = 0;

        if (flags & STAGE_VERTEX) 
            vkflags |= VK_SHADER_STAGE_VERTEX_BIT;
        if (flags & STAGE_FRAGMENT) 
            vkflags |= VK_SHADER_STAGE_FRAGMENT_BIT;
        if (flags & STAGE_COMPUTE) 
            vkflags |= VK_SHADER_STAGE_COMPUTE_BIT;
        if (flags & STAGE_GEOMETRY) 
            vkflags |= VK_SHADER_STAGE_GEOMETRY_BIT;
        if (flags & STAGE_TESSELLATION_CONTROL) 
            vkflags |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        if (flags & STAGE_TESSELLATION_EVALUATION) 
            vkflags |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

        return vkflags;
    }

    inline constexpr StageFlags getSLRDStageFlags (VkShaderStageFlags vkflags) {
        StageFlags flags = 0;

        if (vkflags & VK_SHADER_STAGE_VERTEX_BIT) {
            flags |= STAGE_VERTEX; 
        }
        if (vkflags & VK_SHADER_STAGE_FRAGMENT_BIT) {
            flags |= STAGE_FRAGMENT; 
        }
        if (vkflags & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            flags |= STAGE_TESSELLATION_CONTROL; 
        }
        if (vkflags & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) {
            flags |= STAGE_TESSELLATION_EVALUATION; 
        }
        if (vkflags & VK_SHADER_STAGE_GEOMETRY_BIT) {
            flags |= STAGE_GEOMETRY; 
        }
        if (vkflags & VK_SHADER_STAGE_COMPUTE_BIT) {
            flags |= STAGE_COMPUTE; 
        }

        return flags;
    }

    class VKPipelineLayout {
    public:
        static constexpr uint32_t MAX_SETS = 4;

    private:
        VKDevice *m_device = nullptr;

        VkPipelineLayout m_layout = VK_NULL_HANDLE;

        /* Layouts for every set that were present in the description for this
         * layout */
        std::array<VkDescriptorSetLayout, MAX_SETS> m_setLayouts;

        /* Each PipelineLayout has a key containing information what and how many
         * descriptors it can allocate */
        PoolKey m_key;

    public:
        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] VkPipelineLayout getLayout () const {
            return m_layout;
        }

        [[nodiscard]] const auto& getPoolKey () const {
            return m_key;
        }

        [[nodiscard]] const auto& getSetLayouts () const {
            return m_setLayouts;
        }

        int init (VKDevice *device, const PipelineLayoutInfo& info);
        /* Allocate a uniform set using this pipeline */
        std::shared_ptr<VKUniformSet> allocateUniformSet (uint32_t set);

        std::vector<VkDescriptorSetLayout> getVkSetLayouts (
            const ProxyArray<DescriptorSet>& sets);

        void clearSetLayouts ();

        VKPipelineLayout () {}
        ~VKPipelineLayout ();
    };
};

#endif /* #define __SLRD_VULKAN_PIPELINE_LAYOUT_HPP__ */
