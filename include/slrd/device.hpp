/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_DEVICE_HPP__
#define __SLRD_DEVICE_HPP__

#include <memory>
#include "util/proxyarray.hpp"

namespace slrd {
    struct DeviceConfig {
        slrd::ProxyArray<const char *> device_extensions;
        bool debug = false;

        DeviceConfig () = default;
    };

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
    class ISampler;

    struct SamplerInfo;
    struct SwapchainInfo;
    struct TextureInfo;
    struct ShaderInfo;
    struct BufferInfo;
    struct RenderPassInfo;
    struct GraphicsPipelineInfo;
    struct PipelineLayoutInfo;
    struct CommandQueueInfo;
    struct ComputePipelineInfo;

    /* The logical device interface */
    class IDevice {
    public:
        virtual ~IDevice () = default;

        virtual std::shared_ptr<ISwapchain> createSwapchain (const SwapchainInfo&) = 0;
        virtual std::shared_ptr<ITexture> createTexture (const TextureInfo& info) = 0;
        virtual std::shared_ptr<IShader> createShader (const ShaderInfo& info) = 0;

        virtual std::shared_ptr<IBuffer> createBuffer (const BufferInfo& info) = 0;

        virtual std::shared_ptr<IRenderPass> createRenderPass (const RenderPassInfo&) = 0;
        virtual std::shared_ptr<IFence> createFence (bool signalled = false) = 0;

        virtual std::shared_ptr<IPipeline> createGraphicsPipeline (const GraphicsPipelineInfo& info) = 0;
        virtual std::shared_ptr<IPipeline> createComputePipeline (const ComputePipelineInfo& info) = 0;

        virtual std::shared_ptr<ICommandQueue> createCommandQueue (const CommandQueueInfo& info) = 0;

        virtual std::shared_ptr<ISampler> createSampler (const SamplerInfo& info) = 0;

        virtual void waitIdle () = 0;
    };

    /* Create device with the given config for initialized API */
    std::shared_ptr<IDevice> createDevice (const DeviceConfig&);
};

#endif /* #define __SLRD_DEVICE_HPP__ */
