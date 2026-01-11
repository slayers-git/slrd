/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_TYPES_HPP__
#define __SLRD_TYPES_HPP__

#include <memory>
#include <cstdint>
#include <limits>

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

    using DevicePtr = std::shared_ptr<IDevice>;

    using SwapchainPtr      = std::shared_ptr<ISwapchain>;
    using RenderPassPtr     = std::shared_ptr<IRenderPass>;
    using PipelinePtr       = std::shared_ptr<IPipeline>;
    using ShaderPtr         = std::shared_ptr<IShader>;
    using TexturePtr        = std::shared_ptr<ITexture>;
    using TextureViewPtr    = std::shared_ptr<ITextureView>;
    using BufferPtr         = std::shared_ptr<IBuffer>;
    using SurfacePtr        = std::shared_ptr<ISurface>;
    using FencePtr          = std::shared_ptr<IFence>;
    using CommandQueuePtr   = std::shared_ptr<ICommandQueue>;
    using CommandBufferPtr  = std::shared_ptr<ICommandBuffer>;
    using UniformSetPtr     = std::shared_ptr<IUniformSet>;
    using SamplerPtr        = std::shared_ptr<ISampler>;

    using DeviceSize = uint64_t;
    constexpr DeviceSize WHOLE_DEVICE_SIZE = std::numeric_limits<uint64_t>::max ();
};

#endif /* #define __SLRD_TYPES_HPP__ */
