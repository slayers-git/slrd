/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_SWAPCHAIN_HPP__
#define __SLRD_VULKAN_SWAPCHAIN_HPP__

#include "deviceobject.hpp"

#include "slrd/types.hpp"
#include "surface.hpp"
#include "slrd/swapchain.hpp"
#include "vulkan/resource.hpp"
#include "vulkan/texture.hpp"

#include <deque>
#include <vector>
#include <vulkan/vulkan.h>

#include <rocket/rocket.hpp>

namespace slrd {
    class VKRenderPass;

    struct FrameData {
        /* Image index */
        uint32_t idx;

        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore imageAvailable = VK_NULL_HANDLE;
        VkSemaphore renderFinished = VK_NULL_HANDLE;
    };

    SLRD_RESOURCE_DEFINE_TYPE(VKSwapchain, VK_OBJECT_TYPE_SWAPCHAIN_KHR);
    class VKSwapchain :
            public VKDeviceObject<ISwapchain>,
            public VKNamedResource<VKSwapchain> {
    public:
        /* How many images can we request and have in one swapchain */
        static constexpr uint32_t MAX_SWAPCHAIN_IMAGES = 8;

        /* When the swapchain gets invalidated, the subscribers (the dependant
         * RenderPasses) will be notified */
        rocket::thread_safe_signal<void()> swapchainInvalidated;

    private:
        Ref<VKSurface> m_surface;

        uint32_t m_imageCount;
        VkFormat m_format;
        VkColorSpaceKHR m_colorSpace;
        VkPresentModeKHR m_presentMode;
        VkSurfaceTransformFlagBitsKHR m_preTransform;

        /* Is the render semaphore used? */
        bool m_waitForRender = false;

        VkSwapchainKHR m_swapchain;
        /* For managing semaphores */
        std::deque<FrameData> m_frameHistory;
        FrameData *m_currentFrame;

        /* Current image */
        uint32_t m_currentFrameIdx;

        /* Semaphores that can be (re)used for wait and signal */
        std::vector<VkSemaphore> m_freeSemaphores;
        /* Fences that are used in order to infer whether the semaphores can 
         * be recycled */
        std::vector<VkFence>     m_freeFences;

        std::vector<Ref<VKTexture>> m_textures;
        std::vector<Ref<ITextureView>> m_textureViews;

        /* Fetch a free semaphore */
        VkSemaphore getFreeSemaphore ();
        /* Fetch a free fence */
        VkFence     getFreeFence ();

        void releaseSemaphore (VkSemaphore);
        void releaseFence (VkFence);

        void cleanUpFrame (uint32_t id);
        void clearFrames ();

    public:
        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] VkSwapchainKHR getSwapchain () const {
            return m_swapchain;
        }

        [[nodiscard]]
        std::pair<VkSemaphore, VkSemaphore> getSemaphores ();

        void markForRender () {
            m_waitForRender = true;
        }

        int acquireNextImage (uint32_t *next) final;
        ITextureView *getTextureView (uint32_t id) final;
        ITexture *getTexture (uint32_t id) final;

        SwapchainResult present (const PresentInfo& info) final;
        int resize (uint32_t width, uint32_t height) final;

        slrd::Format getFormat () const final;

        /* free the resources */
        void free ();

        /* Initialize the swapchain */
        int init (VKDevice *device, const SwapchainInfo& info);

        /* Create the swapchain (for both recreation and initialization) */
        VkSwapchainKHR create (VkSwapchainKHR old, uint32_t w, uint32_t h);

        ~VKSwapchain ();

        VkSwapchainKHR handle () const {
            return m_swapchain;
        }

        std::string_view getName () const noexcept final;
    };

    inline VKSwapchain *createVKSwapchain (VKDevice *device, const SwapchainInfo& info) {
        return makeResource<VKSwapchain>(device, info);
    }
};

#endif /* #define __SLRD_VULKAN_SWAPCHAIN_HPP__ */
