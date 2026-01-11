/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include <slrd/uniformset.hpp>
namespace slrd {
    /*UniformUpdater& UniformUpdater::updateUniformBuffer (uint32_t binding, std::shared_ptr<IBuffer>& buffer,*/
    /*            size_t size, size_t offset) {*/
    /*    decltype(m_uniformUpdates)::value_type update;*/
    /*    update.type = UNIFORM_UPDATE_TYPE_BUFFER;*/
    /*    update.binding = binding;*/
    /**/
    /*    decltype(m_buffers)::value_type bufferUpdate;*/
    /*    bufferUpdate.size = size;*/
    /*    bufferUpdate.offset = offset;*/
    /*    bufferUpdate.buffer = buffer.get ();*/
    /**/
    /*    m_buffers.emplace_back (std::move (bufferUpdate));*/
    /**/
    /*    update.buffer.size = size;*/
    /*    update.buffer.offset = offset;*/
    /**/
    /*    m_data.push_back (update);*/
    /**/
    /*    return *this;*/
    /*}*/
    /**/
    /*UniformUpdater& UniformUpdater::updateSampler (uint32_t binding, std::shared_ptr<ISampler>& sampler) {*/
    /*    decltype(m_data)::value_type update;*/
    /*    update.type = UNIFORM_UPDATE_TYPE_TEXTURE;*/
    /*    update.binding = binding;*/
    /*    update.texture.sampler = sampler.get ();*/
    /*    update.texture.view = nullptr;*/
    /**/
    /*    m_data.push_back (update);*/
    /*    return *this;*/
    /*}*/
    /**/
    /*UniformUpdater& UniformUpdater::updateTexture (uint32_t binding, ITextureView *texture) {*/
    /*    decltype(m_data)::value_type update;*/
    /*    update.type = UNIFORM_UPDATE_TYPE_TEXTURE;*/
    /*    update.binding = binding;*/
    /*    update.texture.sampler = nullptr;*/
    /*    update.texture.view = texture;*/
    /**/
    /*    m_data.push_back (update);*/
    /*    return *this;*/
    /*}*/
    /**/
    /*UniformUpdater& UniformUpdater::updateCombinedTexture (uint32_t binding, ITextureView *texture,*/
    /*        std::shared_ptr<ISampler> sampler) {*/
    /*    decltype(m_data)::value_type update;*/
    /*    update.type = UNIFORM_UPDATE_TYPE_TEXTURE;*/
    /*    update.binding = binding;*/
    /*    update.texture.sampler = sampler.get ();*/
    /*    update.texture.view = texture;*/
    /**/
    /*    m_data.push_back (update);*/
    /*    return *this;*/
    /*}*/
    /**/
    /*std::vector<UniformUpdateData::UniformUpdate> UniformUpdater::get () const {*/
    /*    return m_data;*/
    /*}*/
}
