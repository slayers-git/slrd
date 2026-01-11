/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_RENDERPASS_HPP__
#define __SLRD_VULKAN_RENDERPASS_HPP__

#include "slrd/renderpass.hpp"
#include "vulkan/factory.hpp"
#include <memory>
#include <array>
#include "vulkan/swapchain.hpp"

#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class VKDevice;
    class VKSwapchain;
    class VKTextureView;

    class ITextureView;

    using RenderPassHash = uint64_t;

    /* Because of the utter retardedness of the Vulkan 1.0 standard, to implement
     * SLRD library without the need for a separate Framebuffer object we do the
     * following:
     *
     * - For Vulkan 1.0 in the renderpass we keep all textures used for drawing,
     *   and a framebuffer that is used for this particular object.
     * - For Vulkan 1.2 (or for platforms with support for imageless-framebuffers)
     *   we use the just one framebuffer instead.
     *
     * The point is that without the dynamic_rendering extension, for every RenderPass
     * object we also store one Framebuffer inside of it.
     * If two different RenderPasses use the same set of textures that will come at an
     * amplified cost.
     * This will remain a thing, until this library supports Vulkan 1.3. */

    SLRD_RESOURCE_DEFINE_TYPE(VKRenderPass);
    class VKRenderPass : public IRenderPass, public VKResource<VKRenderPass> {
    private:
        std::shared_ptr<VKDevice> m_device;
        VkRenderPass m_renderpass = VK_NULL_HANDLE;

        RenderPassHash m_hash;

        /* Which swapchain you'd need to signal, when this renderpass
         * is activated? */
        VKSwapchain *m_swapchain = nullptr;
        /* Swapchain connections */
        rocket::scoped_connection_container m_swapchainConnections;

        /* Was the swapchain recreated? */
        bool m_swapchainRecreated = true;

        uint32_t m_width;
        uint32_t m_height;

        /* The index of a swapchain image
         * FIXME: should be an array */
        uint32_t m_swapchainImageIndex = UINT32_MAX;
        uint32_t m_depthIndex = UINT32_MAX;
        uint32_t m_stencilIndex = UINT32_MAX;

        uint32_t m_colorAttachments = 0;

        /* If the framebuffer/framebuffers with cache have to be recreated for this
         * renderpass */
        mutable bool m_requiresFBRecreation = true;
        /* If the framebuffer has to be created for this, because the
         * presentable attachment has changed */
        mutable bool m_requiresFBForView = true;

        std::vector<const VKTextureView *> m_textureViews;

        /* Current framebuffer */
        VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
        std::unordered_map<const VKTextureView *, VkFramebuffer> m_framebufferMap;
        /* The framebuffers that are used */
        std::array<VkFramebuffer, VKSwapchain::MAX_SWAPCHAIN_IMAGES>
            m_framebuffers = {};

    public:
        VKRenderPass () {}
        ~VKRenderPass ();

        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] VkRenderPass getRenderPass () const {
            return m_renderpass;
        }

        [[nodiscard]] RenderPassHash getHash () const {
            return m_hash;
        }

        [[nodiscard]] auto& getSwapchain () const {
            return m_swapchain;
        }

        [[nodiscard]] uint32_t getWidth () const {
            return m_width;
        }
        [[nodiscard]] uint32_t getHeight () const {
            return m_height;
        }

        [[nodiscard]] const auto& getColorAttachments () const {
            return m_colorAttachments;
        }

        [[nodiscard]] VkFramebuffer getCurrentFramebuffer () const {
            return m_framebuffer;
        }

        [[nodiscard]] bool hasDepth () const {
            return m_depthIndex != UINT32_MAX;
        }

        [[nodiscard]] bool hasStencil () const {
            return m_stencilIndex != UINT32_MAX;
        }

        int init (std::shared_ptr<VKDevice> device,
                const RenderPassInfo& info);

        /* Set texture views */
        int setTextureViews (ProxyArray<std::shared_ptr<ITextureView>> textureViews) final;

        /* Set texture view for one element
         *
         * Note: this will result in framebuffer recreation, which is highly inefficient.
         * However, if the TextureView being replaced is from a swapchain, then the replaced 
         * framebuffer will be stored. */
        int setTextureView (uint32_t index, const std::shared_ptr<ITextureView>& textureView) final;
        int setTextureView (uint32_t index, const ITextureView *textureView) final;

        virtual int setDepthView   (const std::shared_ptr<ITextureView>& textureView) final;
        virtual int setStencilView (const std::shared_ptr<ITextureView>& textureView) final;

        /* Internal use to create framebuffers based on current set of views */
        VkFramebuffer createFramebuffer ();

        /* Internal use to create framebuffers if needed and if it can be done */
        int createFramebufferIfNeeded ();

        void connectToSwapchain ();

        /* Clear framebuffers */
        void clearFramebuffers ();

        /* Signal that the swapchain that this renderpass depends on is/being
         * recreated */
        void signalSwapchainRecreation ();
    };

    inline std::shared_ptr<VKRenderPass> createVKRenderPass (
            std::shared_ptr<VKDevice> device, const RenderPassInfo& info) {
        return makeResource<VKRenderPass> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_RENDERPASS_HPP__ */
