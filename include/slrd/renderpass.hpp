/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_RENDERPASS_HPP__
#define __SLRD_RENDERPASS_HPP__

#include "format.hpp"
#include <optional>
#include <memory>
#include <span>

namespace slrd {
    class ITextureView;

    enum RenderPassAttachmentFlag {
        RENDERPASS_ATTACHMENT_FLAG_NONE = 0,
    };

    enum RenderPassFlag {
        RENDERPASS_FLAG_NONE     = 0,
        /* Should an implicit texture memory barrier be placed before the 
         * renderpass? */
        RENDERPASS_FLAG_SYNC_IN  = 1,
        /* Should an implicit texture memory barrier be placed after the 
         * renderpass? */
        RENDERPASS_FLAG_SYNC_OUT = 2,
    };

    using RenderPassAttachmentFlags = uint32_t;
    using RenderPassFlags = uint32_t;

    struct RenderPassAttachment {
        slrd::Format  format = FORMAT_UNDEFINED;
        slrd::MSAACount msaa = MSAA_COUNT_1;
        slrd::LoadOperation loadOp   = LOAD_OPERATION_DONT_CARE;
        slrd::StoreOperation storeOp = STORE_OPERATION_DONT_CARE;

        slrd::LoadOperation stencilLoadOp = LOAD_OPERATION_DONT_CARE;
        slrd::StoreOperation stencilStoreOp = STORE_OPERATION_DONT_CARE;

        /* The layouts for the texture in this attachment */
        slrd::TextureLayout initialLayout = TEXTURE_LAYOUT_UNDEFINED;
        slrd::TextureLayout finalLayout   = TEXTURE_LAYOUT_AUTO;

        RenderPassAttachmentFlags flags = RENDERPASS_ATTACHMENT_FLAG_NONE;

        /* Is the attachment presentable? (i.e. is the attachment image part of
         * a swapchain) */
        bool presentable = false;
    };

    struct RenderPassInfo {
        std::span<const RenderPassAttachment> colorAttachments;

        RenderPassFlags flags = RENDERPASS_FLAG_NONE;

        std::optional<RenderPassAttachment> depthAttachment = std::nullopt;
        std::optional<RenderPassAttachment> stencilAttachment = std::nullopt;
    };

    class IRenderPass {
    public:
        virtual ~IRenderPass () = default;

        /* Set the textures that will be used in this renderpass 
         *
         * If this renderpass draws to multiple different swapchain images, then
         * you should set the textureView for these attachments to nullptr, then
         * in the rendering code, update them so that framebuffers can be created
         * for them */
        virtual int setTextureViews (std::span<std::shared_ptr<ITextureView>> textureViews) = 0;

        virtual int setDepthView   (const std::shared_ptr<ITextureView>& textureView) = 0;
        virtual int setStencilView (const std::shared_ptr<ITextureView>& textureView) = 0;

        /* Only call if the TextureView you set is part of a swapchain */
        virtual int setTextureView (uint32_t index, const std::shared_ptr<ITextureView>& textureView) = 0;
        virtual int setTextureView (uint32_t index, const ITextureView *textureView) = 0;
    };
};

#endif /* #define __SLRD_RENDERPASS_HPP__ */
