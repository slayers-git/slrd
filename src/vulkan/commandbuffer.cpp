/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "commandbuffer.hpp"
#include "debug.hpp"
#include "commandqueue.hpp"
#include "vulkan/error.hpp"

#include "pipeline.hpp"
#include "buffer.hpp"
#include "renderpass.hpp"
#include "vulkan/format.hpp"
#include "vulkan/pipelinelayout.hpp"
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "uniformset.hpp"

#include "texture.hpp"

namespace slrd {
    static constexpr VkAccessFlags getVkAccessFlags (slrd::MemoryAccessFlags flags) {
        VkAccessFlags access = 0;
        if (flags & slrd::MEMORY_ACCESS_FLAG_READ)
            access |= VK_ACCESS_MEMORY_READ_BIT;
        if (flags & slrd::MEMORY_ACCESS_FLAG_WRITE)
            access |= VK_ACCESS_MEMORY_WRITE_BIT;

        return access;
    }

    static constexpr VkAccessFlags getVkAccessFlagsForLayout (
            slrd::TextureLayout layout) {
        VkAccessFlags access = 0;
        switch (layout) {
            case TEXTURE_LAYOUT_AUTO:
            case TEXTURE_LAYOUT_UNDEFINED:
                access = 0;
                break;
            case TEXTURE_LAYOUT_GENERAL:
                access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
                break;
            case TEXTURE_LAYOUT_COLOR_ATTACHMENT:
                access = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                    VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                break;
            case TEXTURE_LAYOUT_SHADER_READ_ONLY:
                access = VK_ACCESS_SHADER_READ_BIT;
                break;
            case TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
                access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
                    VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            case TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY:
                access = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                break;
            case TEXTURE_LAYOUT_SWAPCHAIN_SRC:
                access = VK_ACCESS_MEMORY_READ_BIT;
                break;
            case TEXTURE_LAYOUT_TRANSFER_SRC:
                access = VK_ACCESS_TRANSFER_READ_BIT;
                break;
            case TEXTURE_LAYOUT_TRANSFER_DST:
                access = VK_ACCESS_TRANSFER_WRITE_BIT;
                break;
        }

        return access;
    }

    static constexpr VkPipelineStageFlags getVkPipelineStageForLayout (
            auto& device,
            slrd::TextureLayout layout) {
        VkPipelineStageFlags stage;

        switch (layout) {
            case TEXTURE_LAYOUT_AUTO:
            case TEXTURE_LAYOUT_UNDEFINED:
                stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                break;
            case TEXTURE_LAYOUT_GENERAL:
            case TEXTURE_LAYOUT_SHADER_READ_ONLY:
                stage = device->getShaderStages ();
                break;
            case TEXTURE_LAYOUT_COLOR_ATTACHMENT:
                stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                break;
            case TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT:
            case TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY:
                stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                    VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                break;
            case TEXTURE_LAYOUT_SWAPCHAIN_SRC:
                stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
                break;
            case TEXTURE_LAYOUT_TRANSFER_SRC:
            case TEXTURE_LAYOUT_TRANSFER_DST:
                stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                break;
        }

        return stage;
    }

    VKCommandBuffer::~VKCommandBuffer () {
        if (m_buffer) {
            vkFreeCommandBuffers (m_queue->getDevice ()->getVkDevice (),
                    m_owningPool, 1, &m_buffer);
        }
    }

    int VKCommandBuffer::init (VKCommandQueue *queue, bool primary) {
        SLRD_ASSERT (queue != nullptr);

        VkCommandBuffer vkbuffer;

        VkCommandBufferAllocateInfo ainfo {};
        ainfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        ainfo.level = primary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
        ainfo.commandPool = queue->getCommandPool ();
        ainfo.commandBufferCount = 1;

        VK_WRAP_RETURN (
                vkAllocateCommandBuffers (queue->getDevice ()->getVkDevice (), &ainfo, &vkbuffer),
                -1);

        m_queue = queue;
        m_owningPool = queue->getCommandPool ();
        m_buffer = vkbuffer;

        return 0;
    }

    void VKCommandBuffer::reset () {
        vkResetCommandBuffer (m_buffer, 0);
        m_swapchainsToSignal.clear ();
    }

    void VKCommandBuffer::begin () {
        VkCommandBufferBeginInfo begInfo {};
        begInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer (m_buffer, &begInfo);
    }

    void VKCommandBuffer::end () {
        m_renderpass = nullptr;
        m_pipeline = nullptr;
        vkEndCommandBuffer (m_buffer);
    }

    void VKCommandBuffer::beginRenderPass (IRenderPass *renderPass,
            const RenderPassBeginInfo& info) {
        SLRD_ASSERT (renderPass != nullptr);
        SLRD_ASSERT (m_renderpass == VK_NULL_HANDLE);

        auto iRenderPass = static_cast<VKRenderPass *>(renderPass);
        VkRenderPass vkrenderPass = iRenderPass->getRenderPass ();

        SLRD_DEBUG_CRIT_IF (
                info.colorClearValues.size () != iRenderPass->getColorAttachments (),
                "The number of clear values in the renderpass is not equal to " \
                "the number of color attachments provided");

        if (iRenderPass->createFramebufferIfNeeded ()) {
            SLRD_DEBUG_CRIT ("Failed to recreate the framebuffer for the renderpass");
        }

        SLRD_ASSERT (iRenderPass->getCurrentFramebuffer ());

        VkRenderPassBeginInfo begInfo {};
        begInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        begInfo.renderPass = vkrenderPass;
        begInfo.framebuffer = iRenderPass->getCurrentFramebuffer ();
        begInfo.renderArea.extent = { iRenderPass->getWidth (), iRenderPass->getHeight () };

        /* If the renderpass depends on an image from a swapchain, synchronize */
        if (iRenderPass->getSwapchain ()) {
            m_swapchainsToSignal.insert (m_swapchainsToSignal.end (),
                    iRenderPass->getSwapchain ());
        }

        bool depthOrStencil = iRenderPass->hasDepth () ||
            iRenderPass->hasStencil ();

        /* TODO: Perhaps not the most efficient way, but C++ doesn't support VLAs and
         * I ain't making a whole inplace_vector class. */
        std::vector<VkClearValue> clearValues (info.colorClearValues.size () + 
                depthOrStencil);
        for (uint32_t i = 0; i < info.colorClearValues.size (); ++i) {
            /* Also not the best way, but ehhh. */
            std::memcpy (&clearValues[i].color, &info.colorClearValues[i],
                    sizeof (clearValues[i].color));
        }
        if (depthOrStencil) {
            auto& dps = clearValues.back ();
            dps.depthStencil.depth = info.depthStencilClearValue.depth;
            dps.depthStencil.stencil = info.depthStencilClearValue.stencil;
        }

        begInfo.clearValueCount = clearValues.size ();
        begInfo.pClearValues = clearValues.data ();

        m_renderpass = iRenderPass;
        vkCmdBeginRenderPass (m_buffer, &begInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    void VKCommandBuffer::endRenderPass () {
        SLRD_COMPLAIN_RETURN_NV (!m_renderpass, "No renderpass is bound");
        
        vkCmdEndRenderPass (m_buffer);
        m_renderpass = nullptr;
    }


    void VKCommandBuffer::bindGraphicsPipeline (IPipeline *pipeline) {
        SLRD_ASSERT (pipeline != nullptr);

        auto *ipipeline = static_cast<VKPipeline *> (pipeline);
        VkPipeline vkpipeline = m_queue->getDevice ()->
            getPipelineManager ()->getOrCreatePipeline (ipipeline->getState (),
                m_renderpass);
        SLRD_DEBUG_CRIT_IF (!vkpipeline, "Failed to create a pipeline for the renderpass!");

        vkCmdBindPipeline (m_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkpipeline);
        m_pipeline = ipipeline;
    }

    void VKCommandBuffer::bindComputePipeline (IPipeline *pipeline) {
        SLRD_ASSERT (pipeline != nullptr);

        VkPipeline vkpipeline = static_cast<VKPipeline *> (pipeline)->getPipeline ();
        vkCmdBindPipeline (m_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, vkpipeline);

        m_pipeline = static_cast<VKPipeline *>(pipeline);
    }

    void VKCommandBuffer::bindVertexBuffer (IBuffer *buffer, uint32_t binding,
        DeviceSize offset) {
        SLRD_ASSERT (buffer != nullptr);

        VkBuffer vkbuffer = static_cast<VKBuffer *> (buffer)->getBuffer ();
        vkCmdBindVertexBuffers (m_buffer, 0, 1, &vkbuffer, &offset);
    }

    void VKCommandBuffer::bindIndexBuffer (IBuffer *buffer,
            IndexType type, DeviceSize offset) {
        SLRD_ASSERT (buffer != nullptr);

        VkBuffer vkbuffer = static_cast<VKBuffer *> (buffer)->getBuffer ();
        vkCmdBindIndexBuffer (m_buffer, vkbuffer, offset, getVkIndexType (type));
    }

    void VKCommandBuffer::setViewport (const Viewport& viewport) {
        VkViewport vkviewport {};
        vkviewport.x = viewport.x;
        vkviewport.y = viewport.y;
        vkviewport.width = viewport.width;
        vkviewport.height = viewport.height;
        vkviewport.minDepth = viewport.minDepth;
        vkviewport.maxDepth = viewport.maxDepth;

        vkCmdSetViewport (m_buffer, 0, 1, &vkviewport);
    }
    void VKCommandBuffer::setScissor (const Scissor& scissor) {
        VkRect2D vkscissor;
        vkscissor.offset.x = scissor.x;
        vkscissor.offset.y = scissor.y;
        vkscissor.extent.width = scissor.w;
        vkscissor.extent.height = scissor.h;

        vkCmdSetScissor (m_buffer, 0, 1, &vkscissor);
    }

    void VKCommandBuffer::pushConstant (std::span<const uint8_t> data, slrd::StageFlags stage,
            uint32_t offset) {
        SLRD_ASSERT (m_pipeline != VK_NULL_HANDLE);
        vkCmdPushConstants (m_buffer, m_pipeline->getPipelineLayout ()->getLayout (), 
                getVkShaderStageFlags (stage), offset, data.size (), (void *)data.data ());
    }

    void VKCommandBuffer::draw (uint32_t vertexCount, uint32_t instanceCount,
            uint32_t firstVertex, uint32_t firstInstance) {
        vkCmdDraw (m_buffer, vertexCount, instanceCount, firstVertex, firstInstance);
    }
    void VKCommandBuffer::drawIndexed (uint32_t indexCount, uint32_t instanceCount,
            uint32_t firstIndex, uint32_t vertexOffset,
            uint32_t firstInstance) {
        vkCmdDrawIndexed (m_buffer, indexCount, instanceCount, 
                firstIndex, vertexOffset, firstInstance);
    }

    /* According to Vulkan's documentation, an image to which the copy is done should have
     * a format that has a VK_FORMAT_FEATURE_TRANSFER_DST_BIT feature set.
     *
     * I am not checking for this. */
    void VKCommandBuffer::copyBufferToImage (const BufferTextureCopyInfo& info) {

        /* FIXME FIXME FIXME: this should ideally be handled by the code that executes the buffer.
         * All commands should be written to an internal queue, and there should be barriers 
         * where they need to be to avoid putting multiple barriers for the same texture. */

        SLRD_ASSERT (info.buffer && info.texture);
        auto vkbuffer = static_cast<const VKBuffer *>(info.buffer)->getBuffer ();
        auto iTexture = static_cast<VKTexture *>(info.texture);
        auto vktexture = iTexture->getImage ();

        int64_t minLayer = INT32_MAX;
        int64_t maxLayer = INT32_MIN;
        int64_t minMipmap = INT32_MAX;
        int64_t maxMipmap = INT32_MIN;

        std::vector<VkBufferImageCopy> regions (info.regions.size ());
        for (uint32_t i = 0; i < info.regions.size (); ++i) {
            const auto& cur = info.regions[i];
            
            regions[i].bufferOffset = cur.offset;
            regions[i].bufferRowLength = cur.rows;
            regions[i].bufferImageHeight = cur.height;
            regions[i].imageSubresource.mipLevel = cur.textureViewInfo.mipLevel;
            regions[i].imageSubresource.layerCount = cur.textureViewInfo.arrayLayers;
            regions[i].imageSubresource.baseArrayLayer = cur.textureViewInfo.arrayLayer;

            VkImageAspectFlags curAspectMask = getVkTextureAspectFlags (cur.textureViewInfo.aspect);
            regions[i].imageSubresource.aspectMask = curAspectMask;

            regions[i].imageOffset = VkOffset3D{ (int32_t)cur.rect.x,
                (int32_t)cur.rect.y, (int32_t)cur.rect.z };
            regions[i].imageExtent = VkExtent3D{ cur.rect.w,
                cur.rect.h, cur.rect.d };

            minMipmap = std::min (minMipmap, (int64_t)cur.textureViewInfo.mipLevel);
            maxMipmap = std::max (maxMipmap, (int64_t)cur.textureViewInfo.mipLevel +
                    cur.textureViewInfo.mipLevels - 1);

            minLayer = std::min (minLayer, (int64_t)cur.textureViewInfo.arrayLayer);
            maxLayer = std::max (maxLayer, (int64_t)cur.textureViewInfo.arrayLayer +
                    cur.textureViewInfo.arrayLayers - 1);
        }

        SLRD_ASSERT (vkbuffer != VK_NULL_HANDLE && vktexture != VK_NULL_HANDLE);

        vkCmdCopyBufferToImage (m_buffer, vkbuffer, vktexture,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size (), regions.data ());
    };

    void VKCommandBuffer::copyBuffer (const BufferCopyInfo& info) {
        SLRD_ASSERT (info.srcBuffer && info.dstBuffer);

        const VKBuffer *iSrcBuffer = static_cast<const VKBuffer *> (info.srcBuffer);
        VKBuffer *iDstBuffer = static_cast<VKBuffer *> (info.dstBuffer);
        
        VkBuffer src = iSrcBuffer->getBuffer ();
        VkBuffer dst = iDstBuffer->getBuffer ();

        SLRD_ASSERT (src != VK_NULL_HANDLE && dst != VK_NULL_HANDLE);

        VkBufferCopy region;
        region.size = info.size;
        region.srcOffset = info.srcOffset;
        region.dstOffset = info.dstOffset;

        vkCmdCopyBuffer (m_buffer, src, dst, 1, &region);
    }

    void VKCommandBuffer::bindSets (std::span<IUniformSet *> uniformSets,
            uint32_t firstSet) {
        SLRD_ASSERT (m_pipeline != nullptr);

        /* FIXME: Bake this on the UniformSet's side */
        std::vector<VkDescriptorSet> sets (uniformSets.size ());
        for (uint32_t i = 0; i < uniformSets.size (); ++i) {
            auto *uniformSet = static_cast<VKUniformSet *> (uniformSets[i]);
            sets[i] = uniformSet->getDescriptorSet ();
        }

        vkCmdBindDescriptorSets (m_buffer, m_pipeline->getBindPoint (),
                m_pipeline->getPipelineLayout ()->getLayout (), firstSet, uniformSets.size (), sets.data (),
                0, nullptr);
    }

    void VKCommandBuffer::pipelineTextureBarrier (const TextureBarrierInfo& info) {
        SLRD_ASSERT (info.texture);

        VKTexture *iTexture = (VKTexture *)info.texture;
        VkImage vktexture = iTexture->getImage ();

        VkImageMemoryBarrier barrier {};

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = vktexture;
        barrier.oldLayout = getVkTextureLayout (info.currentTextureLayout);
        barrier.newLayout = getVkTextureLayout (info.newTextureLayout);

        barrier.srcAccessMask = getVkAccessFlagsForLayout (info.currentTextureLayout);
        barrier.dstAccessMask = getVkAccessFlagsForLayout (info.newTextureLayout);

        barrier.subresourceRange.aspectMask = getVkTextureAspectFlags (info.viewInfo.aspect);
        barrier.subresourceRange.layerCount = info.viewInfo.arrayLayers;
        barrier.subresourceRange.levelCount = info.viewInfo.mipLevels;

        barrier.subresourceRange.baseArrayLayer = info.viewInfo.arrayLayer;
        barrier.subresourceRange.baseMipLevel = info.viewInfo.mipLevel;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        VkPipelineStageFlags srcStage = getVkPipelineStageForLayout (m_queue->getDevice (),
                                    info.currentTextureLayout),
                             dstStage = getVkPipelineStageForLayout (m_queue->getDevice (),
                                    info.newTextureLayout);

        vkCmdPipelineBarrier (m_buffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    }

    void VKCommandBuffer::pipelineBufferBarrier (const BufferBarrierInfo& info) {
        SLRD_ASSERT (info.buffer && info.size != 0);

        VKBuffer *iBuffer = static_cast<VKBuffer *> (info.buffer);
        SLRD_ASSERT (iBuffer->getBuffer ());

        VkBuffer vkbuffer = iBuffer->getBuffer ();

        VkBufferMemoryBarrier barrier {};
        barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        barrier.buffer = vkbuffer;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.offset = info.offset;
        barrier.size   = info.size;

        barrier.srcAccessMask = getVkAccessFlags (info.srcAccessFlags);
        barrier.dstAccessMask = getVkAccessFlags (info.dstAccessFlags);

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                             dstStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        vkCmdPipelineBarrier (m_buffer, srcStage, dstStage, 0, 0, nullptr, 0,
                &barrier, 0, nullptr);
    }

    void VKCommandBuffer::dispatch (const DispatchInfo& info) {
        SLRD_ASSERT (m_buffer != VK_NULL_HANDLE);
        vkCmdDispatch (m_buffer, info.x, info.y, info.z);
    }
};
