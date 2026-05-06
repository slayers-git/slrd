/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "pipelinelayout.hpp"
#include "debug.hpp"
#include "device.hpp"
#include "vulkan/api.hpp"
#include "vulkan/error.hpp"
#include <vector>

#include "uniformset.hpp"

namespace slrd {
    /* FIXME: Not the best way to deal with this, since we can have multiple 
     * layouts using the same shaders. */
    std::vector<VkDescriptorSetLayout> VKPipelineLayout::getVkSetLayouts (
            std::span<const DescriptorSet> sets) {
        PoolKey poolKey {};

        std::vector<VkDescriptorSetLayout> layouts;
        for (uint32_t i = 0; i < sets.size (); ++i) {
            SLRD_COMPLAIN_RETURN (sets[i].set >= MAX_SETS,
                    std::vector<VkDescriptorSetLayout> (),
                    "Shader has a set number higher than {}", MAX_SETS);

            for (uint32_t j = 0; j < sets[i].bindings.size (); ++j) {
                poolKey.m_array[sets[i].bindings[j].descriptorType] += 
                    sets[i].bindings[j].descriptorCount * 16;
            }

            VkDescriptorSetLayoutCreateInfo dscInfo {};
            dscInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            dscInfo.bindingCount = sets[i].bindings.size ();
            dscInfo.pBindings = sets[i].bindings.data ();

            VkDescriptorSetLayout layout;
            VkResult result;
            if ((result = vkCreateDescriptorSetLayout (m_device->getVkDevice (),
                            &dscInfo, nullptr, &layout))) {
                goto cleanup;
            }

            layouts.push_back (layout);
            m_setLayouts[sets[i].set] = layout;
        }

        m_key = poolKey;

        return layouts;

        /* Clean up on failure */
        cleanup:
            for (auto layout : layouts) {
                vkDestroyDescriptorSetLayout (m_device->getVkDevice (), layout, nullptr);
            }
            clearSetLayouts ();

            return {};
    }

    int VKPipelineLayout::init (VKDevice *device, const PipelineLayoutInfo& info) {
        VkPipelineLayout pipelineLayout;

        m_setLayouts.fill (VK_NULL_HANDLE);
        m_device = device;

        std::vector<VkDescriptorSetLayout> vksets;
        if (!info.sets.empty ()) {
            vksets = getVkSetLayouts (info.sets);
            if (vksets.empty ()) {
                return -1;
            }
        }

        VkPipelineLayoutCreateInfo plInfo {};
        plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        plInfo.pSetLayouts = vksets.data ();
        plInfo.setLayoutCount = vksets.size ();
        if (info.pushConstants.size != 0) {
            plInfo.pPushConstantRanges = &info.pushConstants;
            plInfo.pushConstantRangeCount = 1;
        }

        VK_WRAP_RETURN_LOGERROR (
                vkCreatePipelineLayout (device->getVkDevice (), &plInfo, nullptr, &pipelineLayout),
                -1,
                "Failed to create PipelineLayoutInfo");

        m_layout = pipelineLayout;

        return 0;
    }

    std::shared_ptr<VKUniformSet> VKPipelineLayout::allocateUniformSet (uint32_t set) {
        return makeResource<VKUniformSet> (this, set);
    }

    void VKPipelineLayout::clearSetLayouts () {
        for (auto& layout : m_setLayouts) {
            if (layout != VK_NULL_HANDLE)
                vkDestroyDescriptorSetLayout (m_device->getVkDevice (), layout, nullptr);

            layout = VK_NULL_HANDLE;
        }
    }

    VKPipelineLayout::~VKPipelineLayout () {
        clearSetLayouts ();
        if (m_layout != VK_NULL_HANDLE)
            vkDestroyPipelineLayout (m_device->getVkDevice (), m_layout, nullptr);
    }
};
