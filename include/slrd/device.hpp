/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_DEVICE_HPP__
#define __SLRD_DEVICE_HPP__

#include "types.hpp"
#include <span>

#include "profiler.hpp"

namespace slrd {
    enum DeviceDebugFlag {
        DEVICE_DEBUG_FLAG_NONE = 0,
        /**
         * Enable resource profiling for this device */
        DEVICE_DEBUG_FLAG_RESOURCE_PROFILER = 1,
        /**
         * Enable underlying API resource profiling for this device */
        DEVICE_DEBUG_FLAG_API_RESOURCE_PROFILER = 2,
    };

    struct DeviceConfig {
        std::span<const char *> device_extensions;

        bool debug = false;
        uint32_t debug_flags = DEVICE_DEBUG_FLAG_NONE;

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
    class IDevice : public IObject {
    public:
        virtual ~IDevice () = default;

        virtual Ref<ISwapchain> createSwapchain (const SwapchainInfo&) = 0;
        virtual Ref<ITexture> createTexture (const TextureInfo& info) = 0;
        virtual Ref<IShader> createShader (const ShaderInfo& info) = 0;

        virtual Ref<IBuffer> createBuffer (const BufferInfo& info) = 0;

        virtual Ref<IRenderPass> createRenderPass (const RenderPassInfo&) = 0;
        virtual Ref<IFence> createFence (bool signalled = false) = 0;

        virtual Ref<IPipeline> createGraphicsPipeline (const GraphicsPipelineInfo& info) = 0;
        virtual Ref<IPipeline> createComputePipeline (const ComputePipelineInfo& info) = 0;

        virtual Ref<ICommandQueue> createCommandQueue (const CommandQueueInfo& info) = 0;

        virtual Ref<ISampler> createSampler (const SamplerInfo& info) = 0;

        virtual void waitIdle () = 0;

        /**
         * Get ResourceProfiler for this device 
         *
         * @note This method will return nullptr, if the device wasn't created
         *       with resource profiling turned on or if it was disabled at
         *       library compile time */
        virtual const ResourceProfiler *getResourceProfiler () const noexcept = 0;
    };

    /* Create device with the given config for initialized API */
    Ref<IDevice> createDevice (const DeviceConfig&);
};

#endif /* #define __SLRD_DEVICE_HPP__ */
