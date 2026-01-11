/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "uniformset.hpp"
#include "debug.hpp"
#include "pipeline.hpp"
#include "pipelinelayout.hpp"
#include "vulkan/buffer.hpp"
#include <vulkan/vulkan_core.h>

#include "sampler.hpp"
#include "texture.hpp"
#include "buffer.hpp"
#include "vulkan/format.hpp"

namespace slrd {
    int VKUniformSet::updateUniforms (const UniformUpdateData& data) {
        const auto& updates = data.data;

        size_t nr_image_infos = 0;
        size_t nr_buffer_infos = 0;
        for (const auto& update : updates) {
            if (update.type == UNIFORM_UPDATE_TYPE_TEXTURE) {
                nr_image_infos += update.textures.size ();
            } else {
                nr_buffer_infos += update.buffers.size ();
            }
        }

        std::vector<VkDescriptorImageInfo>  image_infos (nr_image_infos);
        std::vector<VkDescriptorBufferInfo> buffer_infos (nr_buffer_infos);
        std::vector<VkWriteDescriptorSet> writes (updates.size ());

        size_t image_infos_ptr  = 0;
        size_t buffer_infos_ptr = 0;

        for (uint32_t i = 0; i < updates.size (); ++i) {
            const auto& update = updates[i];

            writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            writes[i].dstSet = m_set;
            writes[i].dstBinding = update.binding;
            writes[i].dstArrayElement = update.arrayElement;
            writes[i].descriptorType  = getVkDescriptorType (update.bindingType);

            if (updates[i].type == UNIFORM_UPDATE_TYPE_TEXTURE) {
                for (uint32_t j = 0; j < update.textures.size (); ++j) {
                    const auto& texture_update = update.textures[j];
                    auto& image_info = image_infos[image_infos_ptr + j];

                    SLRD_ASSERT (texture_update.sampler && texture_update.view);

                    VkSampler vksampler = static_cast<VKSampler *> (texture_update.sampler)->getSampler ();
                    VkImageView vkview = static_cast<VKTextureView *> (texture_update.view)->getView ();
                    image_info.sampler = vksampler;
                    image_info.imageView = vkview;
                    image_info.imageLayout = getVkTextureLayout (texture_update.layout);
                }

                writes[i].pImageInfo = image_infos.data () + image_infos_ptr;
                writes[i].descriptorCount = update.textures.size ();

                image_infos_ptr += update.textures.size ();
            } else {
                for (uint32_t j = 0; j < update.buffers.size (); ++j) {
                    const auto& buffer_update = update.buffers[j];
                    auto& buffer_info = buffer_infos[buffer_infos_ptr + j];

                    SLRD_ASSERT (buffer_update.buffer);

                    VkBuffer vkbuffer = static_cast<VKBuffer *> (buffer_update.buffer)->getBuffer ();
                    buffer_info.offset = buffer_update.offset;
                    buffer_info.range  = buffer_update.size;
                    buffer_info.buffer = vkbuffer;
                }

                writes[i].pBufferInfo = buffer_infos.data () + buffer_infos_ptr;
                writes[i].descriptorCount = update.buffers.size ();

                buffer_infos_ptr += update.buffers.size ();
            }
        }

        vkUpdateDescriptorSets (m_device, writes.size (), writes.data (), 0, nullptr);

        return 0;
    }

    int VKUniformSet::init (VKPipelineLayout *pipelineLayout, uint32_t set) {
        SLRD_ASSERT (pipelineLayout && pipelineLayout->getDevice ());

        m_device = pipelineLayout->getDevice ()->getVkDevice ();

        auto descPool = pipelineLayout->getDevice ()->
            allocateOrGetDescriptorManager (pipelineLayout->getPoolKey ());
        if (!descPool) {
            return -1;
        }

        m_manager = descPool;

        std::tie (m_set, m_pool) = descPool->allocateSet (
                pipelineLayout->getSetLayouts ()[set]);
        SLRD_COMPLAIN_IF(!m_set, "Failed to create VKUniformSet!");
        return !m_set;
    }

    VKUniformSet::~VKUniformSet () {
        /* Perhaps there should be some better logics here, but erhhhh */
        m_manager->freeSet (m_pool, m_set);
    }
};
