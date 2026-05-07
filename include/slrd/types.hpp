/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_TYPES_HPP__
#define __SLRD_TYPES_HPP__

#include <memory>
#include <cstdint>
#include <limits>

#include "object.hpp"
#include "ref.hpp"

namespace slrd {
    class IDevice;
    class ISwapchain;
    class IRenderPass;
    class IShader;
    class IPipeline;
    class IBuffer;
    class ITexture;
    class ISurface;
    class IFence;
    class ICommandQueue;
    class ICommandBuffer;
    class IUniformSet;
    class ISampler;
    class ITextureView;

    using DevicePtr = Ref<IDevice>;

    using SwapchainPtr      = Ref<ISwapchain>;
    using RenderPassPtr     = Ref<IRenderPass>;
    using PipelinePtr       = Ref<IPipeline>;
    using ShaderPtr         = Ref<IShader>;
    using TexturePtr        = Ref<ITexture>;
    using TextureViewPtr    = Ref<ITextureView>;
    using BufferPtr         = Ref<IBuffer>;
    using SurfacePtr        = Ref<ISurface>;
    using FencePtr          = Ref<IFence>;
    using CommandQueuePtr   = Ref<ICommandQueue>;
    using CommandBufferPtr  = Ref<ICommandBuffer>;
    using UniformSetPtr     = Ref<IUniformSet>;
    using SamplerPtr        = Ref<ISampler>;

    using DeviceSize = uint64_t;
    constexpr DeviceSize WHOLE_DEVICE_SIZE = std::numeric_limits<uint64_t>::max ();
};

#endif /* #define __SLRD_TYPES_HPP__ */
