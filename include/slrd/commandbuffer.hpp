/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_COMMAND_BUFFER_HPP__
#define __SLRD_COMMAND_BUFFER_HPP__

#include "slrd/format.hpp"
#include "slrd/texture.hpp"
#include "slrd/types.hpp"
#include "slrd/util/proxyarray.hpp"
#include <memory>

namespace slrd {
    class IPipeline;
    class IBuffer;
    class IRenderPass;
    class IFence;
    class IUniformSet;
    class ITexture;

    enum MemoryAccessFlag : uint64_t {
        MEMORY_ACCESS_FLAG_NONE  = 0,
        MEMORY_ACCESS_FLAG_READ  = 1,
        MEMORY_ACCESS_FLAG_WRITE = 2,
    };

    using MemoryAccessFlags = uint64_t;

    struct BufferTextureRegion {
        /* The mip and layer of the texture to which the copy is due */
        TextureViewInfo textureViewInfo;

        uint32_t rows   = 0;
        uint32_t height = 0;

        DeviceSize offset = 0;

        /* The portion of the texture where to copy */
        Rect3D<uint32_t> rect;
    };

    struct BufferTextureCopyInfo {
        /* The source buffer */
        const IBuffer *buffer = nullptr;
        /* The destination texture */
        ITexture *texture     = nullptr;

        slrd::ProxyArray<BufferTextureRegion> regions;
    };

    struct BufferCopyInfo {
        const IBuffer *srcBuffer = nullptr;
        IBuffer *dstBuffer = nullptr;

        DeviceSize srcOffset = 0;
        DeviceSize dstOffset = 0;
        DeviceSize size;
    };

    struct TextureBarrierInfo {
        ITexture *texture = nullptr;
        TextureViewInfo viewInfo;

        TextureLayout currentTextureLayout = slrd::TEXTURE_LAYOUT_UNDEFINED;
        TextureLayout newTextureLayout     = slrd::TEXTURE_LAYOUT_UNDEFINED;
    };

    struct BufferBarrierInfo {
        IBuffer *buffer = nullptr;

        DeviceSize offset = 0;
        DeviceSize size   = 0;

        MemoryAccessFlags srcAccessFlags = MEMORY_ACCESS_FLAG_NONE;
        MemoryAccessFlags dstAccessFlags = MEMORY_ACCESS_FLAG_NONE;
    };

    struct DispatchInfo {
        uint32_t x, y, z;
    };

    struct RenderPassColorClearValue {
        union {
            uint32_t uint32[4];
            int32_t  int32[4];
            float    float32[4];
        };

        RenderPassColorClearValue () = default;
        RenderPassColorClearValue (float r, float g, float b, float a = {}) :
            float32 {r, g, b, a} {}
        RenderPassColorClearValue (uint32_t r, uint32_t g, uint32_t b, uint32_t a = {}) :
            uint32 {r, g, b, a} {}
        RenderPassColorClearValue (int32_t r, int32_t g, int32_t b, int32_t a = int32_t()) :
            int32 {r, g, b, a} {}
    };

    struct RenderPassDepthStencilClearValue {
        float depth = 1.f;
        uint32_t stencil = 0;
    };

    struct RenderPassBeginInfo {
        ProxyArray<const RenderPassColorClearValue> colorClearValues;
        RenderPassDepthStencilClearValue depthStencilClearValue {};
    };

    class ICommandBuffer {
    public:
        virtual ~ICommandBuffer () = default;

        virtual void reset () = 0;

        virtual void begin () = 0;
        virtual void end ()   = 0;

        virtual void beginRenderPass (std::shared_ptr<IRenderPass>& renderPass, const RenderPassBeginInfo&) = 0;
        virtual void endRenderPass () = 0;

        virtual void pushConstant (const ProxyArray<uint8_t>& data, slrd::StageFlags stage,
                uint32_t offset = 0) = 0;

        virtual void bindGraphicsPipeline (std::shared_ptr<IPipeline>& pipeline) = 0;
        virtual void bindComputePipeline (std::shared_ptr<IPipeline>& pipeline) = 0;

        virtual void bindVertexBuffer (std::shared_ptr<IBuffer>& buffer, uint32_t binding,
                uint64_t offset = 0) = 0;
        virtual void bindIndexBuffer (std::shared_ptr<IBuffer>& buffer,
                IndexType type = INDEX_TYPE_UINT16, DeviceSize offset = 0) = 0;

        virtual void setViewport (const Viewport& viewport) = 0;
        virtual void setScissor (const Scissor& scissor)    = 0;

        virtual void bindSets (
                const slrd::ProxyArray<IUniformSet *>& uniformSet, uint32_t firstSet = 0) = 0;

        virtual void draw (uint32_t vertexCount, uint32_t instanceCount,
                uint32_t firstVertex = 0, uint32_t firstInstance = 0) = 0;
        virtual void drawIndexed (uint32_t indexCount, uint32_t instanceCount,
                uint32_t firstIndex = 0, uint32_t vertexOffset = 0,
                uint32_t firstInstance = 0) = 0;

        virtual void copyBufferToImage (const BufferTextureCopyInfo& info) = 0;
        virtual void copyBuffer (const BufferCopyInfo& info) = 0;

        virtual void pipelineTextureBarrier (const TextureBarrierInfo& info) = 0;
        virtual void pipelineBufferBarrier (const BufferBarrierInfo& info) = 0;

        virtual void dispatch (const DispatchInfo&) = 0;
    };
};

#endif /* #define __SLRD_COMMAND_BUFFER_HPP__ */
