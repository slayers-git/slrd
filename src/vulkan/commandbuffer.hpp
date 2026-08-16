/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_COMMAND_BUFFER_HPP__
#define __SLRD_VULKAN_COMMAND_BUFFER_HPP__

#include <rocket/rocket.hpp>
#include "vulkan/deviceobject.hpp"
#include "vulkan/resource.hpp"
#include <slrd/commandbuffer.hpp>
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;
    class VKCommandQueue;
    class VKSwapchain;
    class VKRenderPass;
    class VKPipeline;
    class VKCommandPool;

    class ITexture;

    SLRD_RESOURCE_DEFINE_TYPE(VKCommandBuffer, VK_OBJECT_TYPE_COMMAND_BUFFER);
    class VKCommandBuffer :
            public VKDeviceObject<ICommandBuffer>,
            public VKResource<VKCommandBuffer> {
    public:
        rocket::scoped_connection_container m_poolConnections;

    private:
        Ref<VKCommandPool> m_pool;

        /* The list of swapchains we should signal to that the rendering
         * is finished */
        std::vector<VKSwapchain *> m_swapchainsToSignal;

        VkCommandBuffer m_buffer = VK_NULL_HANDLE;
        VkCommandPool   m_owningPool = VK_NULL_HANDLE;

        /* Current RenderPass */
        VKRenderPass *m_renderpass = nullptr;
        /* Current Pipeline */
        VKPipeline *m_pipeline = nullptr;

        void signalReset ();

#if SLRD_DEBUG
        enum State {
            STATE_INITIAL,
            STATE_RECORDING,
            STATE_EXECUTABLE
        };

        State m_state = STATE_INITIAL;
#endif
        
    public:
        VKCommandBuffer () = default;
        ~VKCommandBuffer ();

        [[nodiscard]] VkCommandBuffer getCommandBuffer () const {
            return m_buffer;
        }

        [[nodiscard]] const auto& getSwapchainsToSingal () const {
            return m_swapchainsToSignal;
        }

        int init (VKCommandPool *pool, const CommandBufferInfo& info);

        void reset () final override;

        void begin () final override;
        void end () final override;

        void beginRenderPass (IRenderPass *, const RenderPassBeginInfo& info) final override;
        void endRenderPass () final override;

        void bindGraphicsPipeline (IPipeline *pipeline) final override;
        void bindComputePipeline (IPipeline *pipeline) final override;

        void bindVertexBuffer (IBuffer *buffer, uint32_t binding,
            DeviceSize offset) final override;
        void bindIndexBuffer (IBuffer *buffer,
            IndexType type, DeviceSize offset) final override;

        void pushConstant (std::span<const uint8_t> data, slrd::StageFlags stage,
                uint32_t offset = 0) final override;

        void setViewport (const Viewport& viewport) final override;
        void setScissor (const Scissor& scissor)    final override;

        void bindSets (std::span<IUniformSet *> uniformSet,
                uint32_t firstSet) final;

        void draw (uint32_t vertexCount, uint32_t instanceCount,
                uint32_t firstVertex = 0, uint32_t firstInstance = 0) final override;
        void drawIndexed (uint32_t indexCound, uint32_t instanceCount,
                uint32_t firstIndex = 0, uint32_t vertexOffset = 0,
                uint32_t firstInstance = 0) final override;

        void copyBufferToImage (const BufferTextureCopyInfo& info) final override;
        void copyBuffer (const BufferCopyInfo& info) final;

        void pipelineTextureBarrier (const TextureBarrierInfo& info) final override;
        void pipelineBufferBarrier (const BufferBarrierInfo& info) final override;

        void blitTexture(const TextureBlitInfo& info) final override;

        void dispatch (const DispatchInfo&) final override;



        VkCommandBuffer handle () const {
            return m_buffer;
        }

    private:
        void transition (VkImageLayout oldLayout, VkImageLayout newLayout);
    };
};

#endif /* #define __SLRD_VULKAN_COMMAND_BUFFER_HPP__ */
