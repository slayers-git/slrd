/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_PLATFORM_VULKAN_HPP__
#define __SLRD_PLATFORM_VULKAN_HPP__

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace slrd {
    class IDevice;
    class ICommandQueue;
    class ICommandBuffer;
    class IRenderPass;
    class ITexture;
    class ITextureView;
    class ISampler;
    class ISwapchain;

    /**
     * Global state that is used by the Vulkan backend */
    struct VulkanData {
        VkInstance instance;

#define __DECLARE_PFN_NAME(__Name) \
        PFN_ ## __Name __Name
        /* The functions we *may* need that are extensions */
        struct VKFunctions {
            __DECLARE_PFN_NAME (vkCreateDebugUtilsMessengerEXT);
            __DECLARE_PFN_NAME (vkDestroyDebugUtilsMessengerEXT);
            __DECLARE_PFN_NAME (vkSetDebugUtilsObjectNameEXT);
        } pfns;
#undef __DECLARE_PFN_NAME
    };

    namespace platform::vulkan {
        /**
         * Get pointer to the global state that is used by Vulkan */
        const VulkanData *getVulkanAPIData ();

        VkDevice getLogicalDevice (IDevice *device);
        VkPhysicalDevice getPhysicalDevice (IDevice *device);

        VkQueue getQueue (ICommandQueue *queue);
        uint32_t getQueueFamily (ICommandQueue *queue);

        VkCommandBuffer getCommandBuffer (ICommandBuffer *commandBuffer);

        VkRenderPass getRenderPass (IRenderPass *renderPass);

        VkImage getTexture (ITexture *texture);
        VkImageView getTextureView (ITextureView *texture);
        VkSampler getSampler (const ISampler *sampler);

        VkSwapchainKHR getSwapchain (ISwapchain *swapchain);
    }
}

#endif /* #define __SLRD_PLATFORM_VULKAN_HPP__ */
