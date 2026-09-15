/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "renderpass.hpp"
#include "device.hpp"
#include "vulkan/format.hpp"
#include <vector>

#include "texture.hpp"

#include "error.hpp"
#include "debug.hpp"

namespace slrd {
    int VKRenderPass::init (VKDevice *device, const RenderPassInfo& info) {
        SLRD_ASSERT(device != nullptr);
        SLRD_ASSERT(info.colorAttachments.size() <= MAX_COLOR_ATTACHMENTS);
        SLRD_DEBUG_CRIT_IF(info.stencilAttachment.has_value(),
                "Separate depth and stencil attachments are not supported");

        /* TODO: Add physical device limits checks */

        VkRenderPass renderpass;

        const auto attachmentCount =
            info.colorAttachments.size () + 
            info.depthAttachment.has_value ();

        /* Attachments */
        std::array<VkAttachmentDescription, MAX_ATTACHMENTS> attachments;
        /* Color references */
        std::array<VkAttachmentReference, MAX_COLOR_ATTACHMENTS> cref;

        /* Depth reference */
        VkAttachmentReference sdref {};

        auto convAttachment = [device](const RenderPassAttachment& attachment, VkAttachmentDescription& result,
                bool depth) -> bool {
            SLRD_COMPLAIN_IF (attachment.format == FORMAT_UNDEFINED,
                    "RenderPass attachment's format shouldn't be undefined.");

            Format format = isDepthFormat (attachment.format) ?
                ::slrd::getFittingDepthFormat (device, attachment.format) :
                attachment.format;
            
            result.format = slrd::getVkFormat (format);
            result.loadOp = slrd::getVkLoadOp (attachment.loadOp);
            result.storeOp = slrd::getVkStoreOp (attachment.storeOp);
            result.stencilLoadOp = slrd::getVkLoadOp (attachment.stencilLoadOp);
            result.stencilStoreOp = slrd::getVkStoreOp (attachment.stencilStoreOp);
            result.samples = slrd::getVkSampleCount (attachment.msaa);

            VkImageLayout finalLayout;
            if (attachment.finalLayout == TEXTURE_LAYOUT_AUTO) {
                if (depth) {
                    finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                } else {
                    finalLayout = attachment.presentable ?
                        VK_IMAGE_LAYOUT_PRESENT_SRC_KHR :
                        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                }
            } else {
                finalLayout = slrd::getVkTextureLayout (attachment.finalLayout);
            }

            if (attachment.initialLayout == TEXTURE_LAYOUT_AUTO) {
                if (attachment.loadOp == LOAD_OPERATION_LOAD) {
                    result.initialLayout = finalLayout;
                } else {
                    result.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                }
            } else {
                result.initialLayout = slrd::getVkTextureLayout (attachment.initialLayout);
            }

            result.finalLayout = finalLayout;
            return true;
        };

        unsigned i;
        for (i = 0; i < info.colorAttachments.size (); ++i) {
            const auto& curInfoColor = info.colorAttachments[i];
            auto& curAttachment = attachments[i];
            
            if (curInfoColor.presentable) {
                /* TODO: Support multiple swapchain image attachments */
                SLRD_ASSERT(m_swapchainImageIndex == UINT32_MAX);
                m_swapchainImageIndex = i;
            }

            if (!convAttachment (curInfoColor, curAttachment, false)) {
                return -1;
            }

            cref[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            cref[i].attachment = i;
        }
        m_colorAttachmentCount = info.colorAttachments.size ();

        VkSubpassDescription subpass {};
        subpass.pColorAttachments = cref.data ();
        subpass.colorAttachmentCount = m_colorAttachmentCount;
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        VkAttachmentDescription ddesc {};
        if (info.depthAttachment.has_value ()) {
            const auto& depth = info.depthAttachment.value ();
            if (!convAttachment (depth, ddesc, true))
                return -1;

            sdref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            sdref.attachment = i;

            m_depthIndex = i;
            if (slrd::hasStencil(info.depthAttachment->format))
                m_stencilIndex = i;

            attachments[i++] = ddesc;
            subpass.pDepthStencilAttachment = &sdref;
        }

        uint32_t subpassDependencyCount = 0;
        std::array<VkSubpassDependency, 2> subpassDependencies;
        if (info.flags & RENDERPASS_FLAG_SYNC_IN) {
            VkSubpassDependency dep {};
            dep.srcSubpass = VK_SUBPASS_EXTERNAL;
            dep.dstSubpass = 0;
            dep.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dep.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dep.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;
            dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;

            subpassDependencies[subpassDependencyCount++] = dep;
        }
        if (info.flags & RENDERPASS_FLAG_SYNC_OUT) {
            VkSubpassDependency dep {};
            dep.srcSubpass = 0;
            dep.dstSubpass = VK_SUBPASS_EXTERNAL;
            dep.srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dep.dstStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dep.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            dep.srcAccessMask = VK_ACCESS_MEMORY_WRITE_BIT;

            subpassDependencies[subpassDependencyCount++] = dep;
        }

        VkRenderPassCreateInfo rpInfo {};
        rpInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        rpInfo.pSubpasses = &subpass;
        rpInfo.subpassCount = 1;
        rpInfo.pAttachments = attachments.data ();
        rpInfo.attachmentCount = attachmentCount;
        rpInfo.dependencyCount = subpassDependencyCount;
        rpInfo.pDependencies = subpassDependencies.data ();

        VK_WRAP_RETURN_RESULT_LOGERROR (
                vkCreateRenderPass (device->getVkDevice (), &rpInfo, nullptr, &renderpass),
                "Failed to create renderpass");

        /* TODO: FIXME calculate the hash based on the rules of RP compatibility */
        m_hash = reinterpret_cast<RenderPassHash> (m_renderpass);

        setParentDevice (device);
        m_renderpass = renderpass;

        if (!info.name.empty ())
            setResourceName (info.name, VK_OBJECT_TYPE_RENDER_PASS, m_renderpass);

        m_requiresFBRecreation = true;
        m_attachmentCount = attachmentCount;

        device->allocate (OBJECT_TYPE_RENDER_PASS, 0);
        device->vkallocate (VK_OBJECT_TYPE_RENDER_PASS, 0);

        return 0;
    }
    
    int VKRenderPass::setTextureViews (std::span<ITextureView *> textureViews) {
        SLRD_ASSERT (textureViews.size () == m_colorAttachmentCount);
        SLRD_ASSERT (m_renderpass != nullptr);

        uint32_t width  = 0;
        uint32_t height = 0;

        for (uint32_t i = 0; i < textureViews.size (); ++i) {
            if (!textureViews[i]) {
                continue;
            }

            auto iTextureView = static_cast<VKTextureView *>(textureViews[i]);
            if (m_textureViews[i] != iTextureView) {
                m_textureViews[i] = iTextureView;
                m_requiresFBRecreation = true;
            }

            SLRD_ASSERT (iTextureView->getTexture () &&
                    iTextureView->getTexture ()->isValid ());
            auto texture = iTextureView->getTexture ();

            /* The size is not uniform */
            SLRD_COMPLAIN_RETURN (i > 0 && !(width == texture->getWidth () &&
                        height == texture->getHeight ()), -1,
                    "RenderPass {}: size is different for attachments!", (void *)m_renderpass);

            width  = texture->getWidth ();
            height = texture->getHeight ();
        }

        return 0;
    }

    VkFramebuffer VKRenderPass::createFramebuffer () {
        SLRD_ASSERT(m_attachmentCount >= 1);
        SLRD_ASSERT(m_textureViews[0]);

        VkFramebuffer vkframebuffer;
        VkFramebufferCreateInfo fbInfo {};
        fbInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fbInfo.renderPass = m_renderpass;

        uint32_t width  = m_textureViews[0]->getTexture()->getWidth();
        uint32_t height = m_textureViews[0]->getTexture()->getHeight();

        std::vector<VkImageView> vkimageViews (m_attachmentCount);
        for (uint32_t i = 0; i < m_attachmentCount; ++i) {
            SLRD_COMPLAIN_RETURN (!m_textureViews[i], VK_NULL_HANDLE,
                    "Creating a framebuffer from an incomplete set of textures");

            SLRD_ASSERT (m_textureViews[i]->getView ());
            vkimageViews[i] = m_textureViews[i]->getView ();

            auto texture = m_textureViews[i]->getTexture();
            SLRD_COMPLAIN_RETURN(
                texture->getWidth() != width || texture->getHeight() != height,
                VK_NULL_HANDLE,
                "Creating a framebuffer with a non-uniformly sized attachments");
        }

        m_width  = width;
        m_height = height;

        fbInfo.width = m_width;
        fbInfo.height = m_height;

        fbInfo.attachmentCount = vkimageViews.size ();
        fbInfo.pAttachments = vkimageViews.data ();
        fbInfo.layers = 1;

        VK_WRAP_RETURN (
                vkCreateFramebuffer (m_device->getVkDevice (), &fbInfo, nullptr, &vkframebuffer),
                VK_NULL_HANDLE);

        /* For swapchain images */
        if (m_swapchainImageIndex != UINT32_MAX) {
            m_framebufferMap.emplace (m_textureViews[m_swapchainImageIndex], vkframebuffer);
        }

        return vkframebuffer;
    }

    void VKRenderPass::connectToSwapchain () {
        m_swapchainConnections += {
            m_swapchain->swapchainInvalidated.connect (this, 
                    &VKRenderPass::signalSwapchainRecreation)
        };
    }

    int VKRenderPass::setTextureView (uint32_t index, ITextureView *textureView) {
        /* Don't allow setting depth textures through this method */
        SLRD_ASSERT (index < m_colorAttachmentCount);

        auto *vkView = static_cast<VKTextureView *>(textureView);
        SLRD_ASSERT (vkView->getTexture () &&
                vkView->getTexture ()->isValid ());

        auto texture = vkView->getTexture ();
        SLRD_COMPLAIN_RETURN (!texture->getSwapchain (), -1,
                "Texture is not bound to a swapchain, use of this function is invalid.");

        if (m_swapchainRecreated) {
            m_swapchain = texture->getSwapchain ();
            m_textureViews[index] = vkView;
            m_swapchainRecreated = false;

            m_swapchainConnections.disconnect ();
            connectToSwapchain ();

            m_requiresFBRecreation = true;
        }

        SLRD_COMPLAIN_RETURN (m_swapchain != texture->getSwapchain (), -1,
                "View is from a different swapchain.");

        if (!m_requiresFBRecreation) {
            auto it = m_framebufferMap.find (vkView);
            if (it != m_framebufferMap.end ()) {
                m_framebuffer = it->second;

                return 0;
            }

        }

        m_textureViews[index] = vkView;
        m_requiresFBForView = true;

        return 0;
    }

    int VKRenderPass::setDepthView (ITextureView *textureView) {
        SLRD_ASSERT (textureView != nullptr);
        SLRD_COMPLAIN_RETURN (m_depthIndex == UINT32_MAX, -1,
                "The RenderPass has no depth attachment");

        auto vkTextureView = static_cast<VKTextureView *> (textureView);
        m_textureViews[m_depthIndex] = vkTextureView;

        if (m_textureViews[m_depthIndex] != vkTextureView) {
            m_requiresFBRecreation = true;
        }

        return 0;
    }
    int VKRenderPass::setStencilView (ITextureView *textureView) {
        SLRD_ASSERT (m_stencilIndex != UINT32_MAX);
        SLRD_COMPLAIN_RETURN (m_stencilIndex == UINT32_MAX, -1,
                "The RenderPass has no stencil attachment");

        auto vkTextureView = static_cast<VKTextureView *> (textureView);
        m_textureViews[m_stencilIndex] = vkTextureView;

        if (m_textureViews[m_stencilIndex] != vkTextureView) {
            m_requiresFBRecreation = true;
        }

        return 0;
    }

    int VKRenderPass::createFramebufferIfNeeded () {
        if (m_requiresFBRecreation) {
            clearFramebuffers ();

            m_framebuffer = createFramebuffer ();
            if (!m_framebuffer)
                return -1;

            m_requiresFBRecreation = false;
            m_requiresFBForView = false;
        } else if (m_requiresFBForView) {
            m_framebuffer = createFramebuffer ();
            if (!m_framebuffer)
                return -1;

            m_requiresFBForView = false;
        }

        return 0;
    }

    void VKRenderPass::clearFramebuffers () {
        for (auto framebuffer : m_framebufferMap) {
            vkDestroyFramebuffer (m_device->getVkDevice (), framebuffer.second, nullptr);
        }
        m_framebufferMap.clear ();

        if (m_swapchainImageIndex == UINT32_MAX) {
            vkDestroyFramebuffer (m_device->getVkDevice (), m_framebuffer, nullptr);
        }

        m_framebuffer = VK_NULL_HANDLE;
    }

    void VKRenderPass::signalSwapchainRecreation () {
        /* Invalidate all framebuffers */
        clearFramebuffers ();
        m_swapchainRecreated = true;
    }

    std::string_view VKRenderPass::getName () const noexcept {
        return getResourceName ();
    }


    VKRenderPass::~VKRenderPass () {
        if (m_renderpass != VK_NULL_HANDLE) {
            vkDestroyRenderPass (m_device->getVkDevice (), m_renderpass, nullptr);

            m_device->deallocate (OBJECT_TYPE_RENDER_PASS, 0);
            m_device->vkdeallocate (VK_OBJECT_TYPE_RENDER_PASS, 0);
        }

        clearFramebuffers ();
    }
};
