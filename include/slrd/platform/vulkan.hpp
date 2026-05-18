/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_PLATFORM_VULKAN_HPP__
#define __SLRD_PLATFORM_VULKAN_HPP__

#include "slrd/profiler.hpp"
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

    class VKDevice;

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


        /**
         * Class that exposes Vulkan resource usage */
        class VKResourceProfiler {
        public:
            /**
             * Query resource usage data for a Vulkan object type */
            const ResourceUsage& query (VkObjectType resource_type) const noexcept;

            VKResourceProfiler () noexcept;
            ~VKResourceProfiler () noexcept = default;
        
        private:
            void allocate (VkObjectType type, VkDeviceSize size) noexcept;
            void deallocate (VkObjectType type, VkDeviceSize size) noexcept;

            std::mutex m_mtx;

            /* We don't care about extension object types */
            static constexpr unsigned MAX_RESOURCE_TYPES = 32;
            ResourceUsage m_resources[MAX_RESOURCE_TYPES];

            friend class slrd::VKDevice;
        };

        /**
         * Get Vulkan resource profiler for the given device 
         *
         * @note The device must be created with respective flags set,
         *       otherwise this function will return nullptr */
        const VKResourceProfiler *getVulkanResourceProfiler (IDevice *device);
    }
}

#endif /* #define __SLRD_PLATFORM_VULKAN_HPP__ */
