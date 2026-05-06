/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_COMMAND_BUFFER_HPP__
#define __SLRD_VULKAN_COMMAND_BUFFER_HPP__

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

    class ITexture;

    SLRD_RESOURCE_DEFINE_TYPE(VKCommandBuffer);
    class VKCommandBuffer : public ICommandBuffer,
            public VKResource<VKCommandBuffer> {
    private:
        /* Queue to which this command buffer belongs */
        std::shared_ptr<VKCommandQueue> m_queue;

        /* The list of swapchains we should signal to that the rendering
         * is finished */
        std::vector<VKSwapchain *> m_swapchainsToSignal;

        VkCommandBuffer m_buffer = VK_NULL_HANDLE;
        VkCommandPool   m_owningPool = VK_NULL_HANDLE;

        /* Current RenderPass */
        VKRenderPass *m_renderpass = nullptr;
        /* Current Pipeline */
        VKPipeline *m_pipeline = nullptr;
        
    public:
        VKCommandBuffer () = default;
        ~VKCommandBuffer ();

        [[nodiscard]] VkCommandBuffer getCommandBuffer () const {
            return m_buffer;
        }

        [[nodiscard]] auto& getCommandQueue () const {
            return m_queue;
        }

        [[nodiscard]] const auto& getSwapchainsToSingal () const {
            return m_swapchainsToSignal;
        }

        int init (std::shared_ptr<VKCommandQueue> queue,
                bool primary);

        void reset () final override;

        void begin () final override;
        void end () final override;

        void beginRenderPass (std::shared_ptr<IRenderPass>&, const RenderPassBeginInfo& info) final override;
        void endRenderPass () final override;

        void bindGraphicsPipeline (std::shared_ptr<IPipeline>& pipeline) final override;
        void bindComputePipeline (std::shared_ptr<IPipeline>& pipeline) final override;

        void bindVertexBuffer (std::shared_ptr<IBuffer>& buffer, uint32_t binding,
            DeviceSize offset) final override;
        void bindIndexBuffer (std::shared_ptr<IBuffer>& buffer,
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

        void dispatch (const DispatchInfo&) final override;




    private:
        void transition (VkImageLayout oldLayout, VkImageLayout newLayout);
    };
};

#endif /* #define __SLRD_VULKAN_COMMAND_BUFFER_HPP__ */
