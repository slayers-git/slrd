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

    struct VulkanData {
        VkInstance instance;

        /* All devices in use */
        std::vector<std::weak_ptr<IDevice>> devices;

#define __DECLARE_PFN_NAME(__Name) \
        PFN_ ## __Name __Name
        /* The functions we *may* need that are extensions */
        struct VKFunctions {
            __DECLARE_PFN_NAME (vkCreateDebugUtilsMessengerEXT);
            __DECLARE_PFN_NAME (vkDestroyDebugUtilsMessengerEXT);
        } pfns;
#undef __DECLARE_PFN_NAME
    };

    namespace platform::vulkan {
        /* Get the data for initialized vulkan api */
        const VulkanData *getVulkanAPIData ();

        VkDevice getLogicalDevice (const std::shared_ptr<IDevice>& device);
        VkPhysicalDevice getPhysicalDevice (const std::shared_ptr<IDevice>& device);

        VkQueue getQueue (const std::shared_ptr<ICommandQueue>& queue);
        uint32_t getQueueFamily (const std::shared_ptr<ICommandQueue>& queue);

        VkCommandBuffer getCommandBuffer (
                const std::shared_ptr<ICommandBuffer>& commandBuffer);

        VkRenderPass getRenderPass (const std::shared_ptr<IRenderPass>& renderPass);

        VkImage getTexture (const std::shared_ptr<ITexture>& texture);
        VkImageView getTextureView (const std::shared_ptr<ITextureView>& texture);
        VkSampler getSampler (const std::shared_ptr<ISampler>& sampler);

        VkSwapchainKHR getSwapchain (const std::shared_ptr<ISwapchain>& swapchain);
    }
}

#endif /* #define __SLRD_PLATFORM_VULKAN_HPP__ */
