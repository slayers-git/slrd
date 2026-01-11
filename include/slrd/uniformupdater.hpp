/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_UNIFORMUPDATER_HPP__
#define __SLRD_UNIFORMUPDATER_HPP__

#include "uniformset.hpp"

#include <vector>

namespace slrd {
    /* A uniform builder convenience class */
    class UniformUpdater {
    private:
        std::vector<UniformUpdateData::UniformUpdate> m_uniformUpdates;
        mutable UniformUpdateData m_data;

    public:
        UniformUpdater& updateUniformBuffer (uint32_t binding, IBuffer *buffer,
                    DeviceSize size, DeviceSize offset = 0) {
            UniformUpdateDataBuffer buffer_data;
            buffer_data.size = size;
            buffer_data.offset = offset;
            buffer_data.buffer = buffer;

            std::vector<UniformUpdateDataBuffer> buffer_updates;
            buffer_updates.emplace_back (std::move (buffer_data));

            UniformUpdateData::UniformUpdate update;
            update.binding = binding;
            update.arrayElement = 0;
            update.bindingType  = BINDING_TYPE_UNIFORM_BUFFER;
            update.type = UNIFORM_UPDATE_TYPE_BUFFER;
            update.buffers = std::move (buffer_updates);

            m_uniformUpdates.emplace_back (std::move (update));
            return *this;
        }

        UniformUpdater& updateStorageBuffer (uint32_t binding, IBuffer *buffer,
                    DeviceSize size, DeviceSize offset = 0) {
            UniformUpdateDataBuffer buffer_data;
            buffer_data.size = size;
            buffer_data.offset = offset;
            buffer_data.buffer = buffer;

            std::vector<UniformUpdateDataBuffer> buffer_updates;
            buffer_updates.emplace_back (std::move (buffer_data));

            UniformUpdateData::UniformUpdate update;
            update.binding = binding;
            update.arrayElement = 0;
            update.bindingType  = BINDING_TYPE_STORAGE_BUFFER;
            update.type = UNIFORM_UPDATE_TYPE_BUFFER;
            update.buffers = std::move (buffer_updates);

            m_uniformUpdates.emplace_back (std::move (update));
            return *this;
        }

        UniformUpdater& updateSampler (uint32_t binding, ISampler *sampler) {
            UniformUpdateDataTexture texture_data;
            texture_data.sampler = nullptr;
            texture_data.view    = nullptr;
            texture_data.layout  = slrd::TEXTURE_LAYOUT_UNDEFINED;

            std::vector<UniformUpdateDataTexture> texture_updates;
            texture_updates.emplace_back (std::move (texture_data));

            UniformUpdateData::UniformUpdate update;
            update.binding = binding;
            update.arrayElement = 0;
            update.bindingType  = BINDING_TYPE_SAMPLER;
            update.type = UNIFORM_UPDATE_TYPE_TEXTURE;
            update.textures = std::move (texture_updates);

            m_uniformUpdates.emplace_back (std::move (update));
            return *this;
        }

        UniformUpdater& updateTexture (uint32_t binding, ITextureView *texture,
                TextureLayout layout) {
            UniformUpdateDataTexture texture_data;
            texture_data.sampler = nullptr;
            texture_data.view    = texture;
            texture_data.layout  = layout;

            std::vector<UniformUpdateDataTexture> texture_updates;
            texture_updates.emplace_back (std::move (texture_data));

            UniformUpdateData::UniformUpdate update;
            update.binding = binding;
            update.arrayElement = 0;
            update.bindingType  = BINDING_TYPE_TEXTURE;
            update.type = UNIFORM_UPDATE_TYPE_TEXTURE;
            update.textures = std::move (texture_updates);

            m_uniformUpdates.emplace_back (std::move (update));
            return *this;
        }

        UniformUpdater& updateCombinedTexture (uint32_t binding, ITextureView *texture,
                ISampler *sampler, TextureLayout layout) {
            UniformUpdateDataTexture texture_data;
            texture_data.sampler = sampler;
            texture_data.view    = texture;
            texture_data.layout  = layout;

            std::vector<UniformUpdateDataTexture> texture_updates;
            texture_updates.emplace_back (std::move (texture_data));

            UniformUpdateData::UniformUpdate update;
            update.binding = binding;
            update.arrayElement = 0;
            update.bindingType  = BINDING_TYPE_COMBINED_TEXTURE_SAMPLER;
            update.type = UNIFORM_UPDATE_TYPE_TEXTURE;
            update.textures = std::move (texture_updates);

            m_uniformUpdates.emplace_back (std::move (update));
            return *this;
        }

        const UniformUpdateData& get () const {
            m_data.data = m_uniformUpdates;
            return m_data;
        }
    };
};

#endif /* #define __SLRD_UNIFORMUPDATER_HPP__ */
