/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "swapchain.hpp"
#include "debug.hpp"
#include "vulkan/error.hpp"
#include <algorithm>
#include <vulkan/vulkan_core.h>

#include "device.hpp"
#include "vulkan/format.hpp"
#include "vulkan/renderpass.hpp"

namespace slrd {
    std::pair<VkSemaphore, VkSemaphore> VKSwapchain::getSemaphores () {
        SLRD_ASSERT (m_currentFrameIdx != UINT32_MAX);
        return std::make_pair (m_currentFrame->imageAvailable,
                m_currentFrame->renderFinished);
    }

    VkSemaphore VKSwapchain::getFreeSemaphore () {
        VkSemaphore sem;

        if (!m_freeSemaphores.empty ()) {
            sem = m_freeSemaphores.back ();
            m_freeSemaphores.pop_back ();
            return sem;
        }

        VkSemaphoreCreateInfo sem_info {};
        sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        VK_WRAP_RETURN_LOGERROR (
                vkCreateSemaphore (m_device->getVkDevice (), &sem_info, nullptr, &sem),
                VK_NULL_HANDLE,
                "VKSwapchain::getFreeSemaphore: Failed to create a semaphore for the swapchain");

        return sem;
    }

    VkFence VKSwapchain::getFreeFence () {
        VkFence fence;

        if (!m_freeFences.empty ()) {
            fence = m_freeFences.back ();
            m_freeFences.pop_back ();
            return fence;
        }

        VkFenceCreateInfo fence_info {};
        fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        VK_WRAP_RETURN_LOGERROR (
                vkCreateFence (m_device->getVkDevice (), &fence_info, nullptr, &fence),
                VK_NULL_HANDLE,
                "VKSwapchain::getFreeFence: Failed to create a fence for the swapchain");

        return fence;
    }
    
    void VKSwapchain::releaseSemaphore (VkSemaphore sem) {
        m_freeSemaphores.push_back (sem);
    }

    void VKSwapchain::releaseFence (VkFence fence) {
        m_freeFences.push_back (fence);
        VK_WRAP_CRASH (vkResetFences (m_device->getVkDevice (), 1, &fence));
    }

    void VKSwapchain::cleanUpFrame (uint32_t id) {
        for (auto frame = m_frameHistory.begin (); frame != m_frameHistory.end ();
                ++frame) {
            if (frame->idx == id) {
                const auto status = vkGetFenceStatus (m_device->getVkDevice (),
                        frame->fence);

                if (status != VK_NOT_READY) {
                    releaseSemaphore (frame->renderFinished);
                    releaseSemaphore (frame->imageAvailable);
                    releaseFence (frame->fence);

                    m_frameHistory.erase (frame);
                    break;
                }
            }
        }
    }

    void VKSwapchain::clearFrames () {
        if (m_frameHistory.empty ())
            return;

        std::vector<VkFence> fences;
        fences.reserve (m_frameHistory.size ());
        for (auto frame : m_frameHistory) {
            fences.emplace_back (frame.fence);
        }

        VK_WRAP_CRASH (vkWaitForFences (m_device->getVkDevice (), fences.size (),
                fences.data (), VK_TRUE, UINT64_MAX));

        for (auto& frame : m_frameHistory) {
            if (frame.imageAvailable)
                releaseSemaphore (frame.imageAvailable);
            if (frame.renderFinished)
                releaseSemaphore (frame.renderFinished);
            if (frame.fence)
                releaseFence (frame.fence);
        }

        m_frameHistory.clear ();
    }

    int VKSwapchain::acquireNextImage (uint32_t *next) {
        SLRD_ASSERT (next != nullptr);

        VkSemaphore image_available_sem = getFreeSemaphore ();
        VkSemaphore render_finished_sem = getFreeSemaphore ();
        VkFence     tracking_fence      = getFreeFence ();

        if (!image_available_sem || !render_finished_sem || !tracking_fence) {
            if (image_available_sem)
                vkDestroySemaphore (m_device->getVkDevice (), image_available_sem,
                        nullptr);
            if (render_finished_sem)
                vkDestroySemaphore (m_device->getVkDevice (), render_finished_sem,
                        nullptr);
            if (tracking_fence)
                vkDestroyFence (m_device->getVkDevice (), tracking_fence,
                        nullptr);

            return SWAPCHAIN_RESULT_OTHER;
        }

        VkResult result = vkAcquireNextImageKHR (m_device->getVkDevice (), m_swapchain, UINT64_MAX, 
                image_available_sem, tracking_fence, next);
        if (result != VK_SUCCESS) {
            releaseFence (tracking_fence);
            releaseSemaphore (image_available_sem);
            releaseSemaphore (render_finished_sem);

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
                return SWAPCHAIN_RESULT_NEEDS_RESIZE;
            }
        }

        cleanUpFrame (*next);

        FrameData frame;
        frame.idx = *next;
        frame.imageAvailable = image_available_sem;
        frame.renderFinished = render_finished_sem;
        frame.fence = tracking_fence;

        m_frameHistory.emplace_back (std::move (frame));
        m_currentFrame = &m_frameHistory.back ();

        m_currentFrameIdx = *next;
        return result == VK_SUCCESS ? SWAPCHAIN_RESULT_SUCCESS : SWAPCHAIN_RESULT_OTHER;
    }

    ITextureView *VKSwapchain::getTextureView (uint32_t id) {
        SLRD_ASSERT (id < m_textures.size ());
        return m_textureViews[id].get ();
    }
    ITexture *VKSwapchain::getTexture (uint32_t id) {
        SLRD_ASSERT (id < m_textures.size ());
        return m_textures[id].get ();
    }

    VkSwapchainKHR VKSwapchain::create (VkSwapchainKHR old, uint32_t width, uint32_t height) {
        auto caps = m_surface->queryCapabilities (m_device.get ());
        const auto& vkcap = caps->capabilities;

        m_currentFrame = nullptr;
        m_currentFrameIdx = UINT32_MAX;
        m_preTransform = vkcap.currentTransform;

        width = std::clamp (width, vkcap.minImageExtent.width, vkcap.maxImageExtent.width);
        height = std::clamp (height, vkcap.minImageExtent.height, vkcap.maxImageExtent.height);

        VkSwapchainCreateInfoKHR scInfo {};
        scInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scInfo.surface = m_surface->getSurface ();
        scInfo.minImageCount = m_imageCount;
        scInfo.imageFormat = m_format;
        scInfo.imageColorSpace = m_colorSpace;
        scInfo.imageExtent = { width, height };
        scInfo.imageArrayLayers = 1;
        scInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scInfo.preTransform = m_preTransform;
        scInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scInfo.presentMode = m_presentMode;
        scInfo.clipped = VK_TRUE;
        scInfo.oldSwapchain = old;

        std::vector<VkImage> images;
        const char *errFn = "";

        auto name = getResourceName ();

        VkSwapchainKHR sc = nullptr;
        if (vkCreateSwapchainKHR (m_device->getVkDevice (), &scInfo, nullptr, &sc) != VK_SUCCESS) {
            errFn = "vkCreateSwapchainKHR";
            goto failed;
        }

        {
            uint32_t image_count = 0;
            if (vkGetSwapchainImagesKHR (m_device->getVkDevice (), sc, &image_count, nullptr) != 
                    VK_SUCCESS) {
                errFn = "vkGetSwapchainImagesKHR";
                goto failed;
            }

            images.resize (image_count);
            if (vkGetSwapchainImagesKHR (m_device->getVkDevice (), sc, &image_count,
                        images.data ()) != VK_SUCCESS) {
                errFn = "vkGetSwapchainImagesKHR";
                goto failed;
            }

            m_imageCount = image_count;
        }

        m_textures.resize (m_imageCount);
        for (uint32_t i = 0; i < m_imageCount; ++i) {
            auto texture = Ref<VKTexture>::adopt (new VKTexture);

            if (texture->createFromExisting (m_device.get (), images[i],
                        TEXTURE_TYPE_2D, scInfo.imageExtent.width,
                        scInfo.imageExtent.height, 1,
                        scInfo.imageFormat, this)) {
                errFn = "VKTexture::createFromExisting";
                goto failed;
            }

            m_textures[i] = std::move (texture);
        }

        m_textureViews.resize (m_imageCount);
        for (uint32_t i = 0; i < m_imageCount; ++i) {
            auto image_view = m_textures[i]->createTextureView ({});
            if (!image_view) {
                errFn = "VKTexture::createTextureView";
                goto failed;
            }

            m_textureViews[i] = std::move (image_view);
        }

        if (!name.empty ())
            setResourceName (name, VK_OBJECT_TYPE_SWAPCHAIN_KHR, sc);

        return sc;

    failed:
        setError ("Failed to create swapchain: {}", errFn);
        vkDestroySwapchainKHR (m_device->getVkDevice (), sc, nullptr);

        m_textureViews.clear ();
        m_textures.clear ();

        return VK_NULL_HANDLE;
    }

    /* Create the swapchain */
    int VKSwapchain::init (VKDevice *device, const SwapchainInfo& info) {
        VKSurface *surface = static_cast<VKSurface *> (info.surface);

        auto caps = surface->queryCapabilities (device);
        if (!caps) {
            slrd::debug::info ("[VKSwapchain] Failed to query capabilities: {}",
                    string_VkResult (caps.error ()));
            return -1;
        }

        const auto& vkcap = caps->capabilities;

        uint32_t imageCount = std::clamp (info.requestedImages,
                vkcap.minImageCount,
                (vkcap.maxImageCount && vkcap.maxImageCount < MAX_SWAPCHAIN_IMAGES) ?
                    vkcap.maxImageCount : MAX_SWAPCHAIN_IMAGES);

        auto surfaceFormat = caps->selectBestFormatAvailable ();
        auto presentMode = caps->selectBestModeAvailable (info.requireVSync);

        m_presentMode = presentMode;
        m_format = surfaceFormat.format;
        m_colorSpace = surfaceFormat.colorSpace;
        m_imageCount = imageCount;
        m_preTransform = vkcap.currentTransform;

        setParentDevice (device);
        m_surface = Ref<VKSurface>::share (surface);
        m_swapchain = create (VK_NULL_HANDLE, info.width, info.height);

        device->allocate (OBJECT_TYPE_SWAPCHAIN, 0);

        return !m_swapchain;
    }

    SwapchainResult VKSwapchain::present (const PresentInfo& info) {
        SLRD_ASSERT (m_swapchain != VK_NULL_HANDLE);

        FrameData& present_data = *m_currentFrame;

        VkPresentInfoKHR pInfo {};
        pInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        pInfo.pSwapchains = &m_swapchain;
        pInfo.pImageIndices = &info.image;
        pInfo.swapchainCount = 1;
        pInfo.waitSemaphoreCount = 1;

        /* We can't wait on the semaphore that is unsignalled, because the 
         * presentation engine will just not be able to catch up. To avoid this
         * if, for some reason, the swapchain is not used in any renderpasses
         * but is presented, we use the imageAvailable semaphore as a go-sign to
         * present the contents of the swapchain */
        if (m_waitForRender) {
            pInfo.pWaitSemaphores = &present_data.renderFinished;
            m_waitForRender = false;
        } else {
            pInfo.pWaitSemaphores = &present_data.imageAvailable;
        }

        auto res = vkQueuePresentKHR (m_device->getPresentQueue (), &pInfo);
        if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) {
            return SWAPCHAIN_RESULT_NEEDS_RESIZE;
        }

        return res == VK_SUCCESS ? SWAPCHAIN_RESULT_SUCCESS : SWAPCHAIN_RESULT_OTHER;
    }

    void VKSwapchain::free () {
        clearFrames ();

        for (auto& sem : m_freeSemaphores) {
            vkDestroySemaphore (m_device->getVkDevice (), sem, nullptr);
        }
        m_freeSemaphores.clear ();

        for (auto& fence : m_freeFences) {
            vkDestroyFence (m_device->getVkDevice (), fence, nullptr);
        }
        m_freeFences.clear ();

        m_device->waitIdle ();
        if (m_swapchain)
            vkDestroySwapchainKHR (m_device->getVkDevice (), m_swapchain, nullptr);

        for (auto& texture : m_textures) {
            VKTexture *vktexture = static_cast<VKTexture *>(texture.get ());
            vktexture->invalidate ();
        }

        m_textures.clear ();
        m_textureViews.clear ();

        m_swapchain = VK_NULL_HANDLE;

        m_currentFrame = nullptr;
        m_currentFrameIdx = UINT32_MAX;
    }

    int VKSwapchain::resize (uint32_t width, uint32_t height) {
        /* Signal to renderpasses that this swapchain's being recreated
         *
         * Also clear the cached framebuffers, because they are invalid */
        swapchainInvalidated ();

        auto old = std::exchange (m_swapchain, VK_NULL_HANDLE);
        free ();

        auto sc = create (old, width, height);
        if (sc) {
            m_swapchain = sc;
        }
        vkDestroySwapchainKHR (m_device->getVkDevice (), old, nullptr);

        return !sc;
    }

    slrd::Format VKSwapchain::getFormat () const {
        return getSLRDFormat (m_format);
    }

    std::string_view VKSwapchain::getName () const noexcept {
        return getResourceName ();
    }

    VKSwapchain::~VKSwapchain () {
        free ();
        m_device->deallocate (OBJECT_TYPE_SWAPCHAIN, 0);
    }
};
