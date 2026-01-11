/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "api.hpp"

#include <algorithm>
#include <cstring>
#include <vector>

#include "error.hpp"
#include <vulkan/vulkan.h>

#include "device.hpp"
#include "commandqueue.hpp"
#include "commandbuffer.hpp"
#include "renderpass.hpp"
#include "texture.hpp"
#include "sampler.hpp"
#include "swapchain.hpp"

static_assert (sizeof (slrd::DeviceSize) == sizeof (VkDeviceSize));
static_assert (slrd::WHOLE_DEVICE_SIZE == VK_WHOLE_SIZE);

namespace slrd {
    std::unique_ptr<VKAPI> vkapi = nullptr;

    static std::vector<VkExtensionProperties> getSupportedExtensions () {
        uint32_t count = 0;
        VK_WRAP_RETURN (
                vkEnumerateInstanceExtensionProperties (nullptr, &count, nullptr),
                {});
        std::vector<VkExtensionProperties> extensions (count);

        VK_WRAP_RETURN (vkEnumerateInstanceExtensionProperties (nullptr, &count, extensions.data ()),
                {});
        return extensions;
    }

    static std::vector<VkLayerProperties> getSupportedLayers () {
        uint32_t count = 0;
        VK_WRAP_RETURN (
                vkEnumerateInstanceLayerProperties (&count, nullptr),
                {});
        std::vector<VkLayerProperties> layers (count);

        VK_WRAP_RETURN (vkEnumerateInstanceLayerProperties (&count, layers.data ()),
                {});
        return layers;
    }

    int initVulkanAPI (const APIConfig& config) {
        if (slrd::getCurrentAPI () != API_NONE)
            return -1;

        vkapi = std::make_unique<VKAPI>();
        return vkapi->init (config);
    }

    void deinitVulkanAPI () {
        if (slrd::getCurrentAPI () != API_VULKAN)
            return;

        vkapi = 0;
    }

    int VKAPI::init (const APIConfig& config) {
        auto supportedExtensions = getSupportedExtensions ();
        auto supportedLayers     = getSupportedLayers ();

        std::vector<const char *> requestedInstanceExtensions (
                config.instance_extensions.begin (), config.instance_extensions.end ());

#if SLRD_VULKAN_DEBUG_MESSENGER_ENABLED
        requestedInstanceExtensions.push_back (VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

        for (auto& extension : requestedInstanceExtensions) {
            bool found = false;
            for (auto& supportedExtension : supportedExtensions) {
                if (strcmp (supportedExtension.extensionName, extension) == 0) {
                    found = true;
                    break;
                }
            }

            RETURN_LOG_ERROR_IF (!found, -1,
                    "Extension {} not present", extension);
        }

        std::vector<const char *> requestedInstanceLayers (config.instance_layers.begin (),
                config.instance_layers.end ());
        if (config.debug) {
            requestedInstanceLayers.push_back ("VK_LAYER_KHRONOS_validation");
        }

        std::sort (requestedInstanceLayers.begin(), requestedInstanceLayers.end());
        requestedInstanceLayers.erase (
                std::unique (requestedInstanceLayers.begin(),
                    requestedInstanceLayers.end()), 
                requestedInstanceLayers.end());

        for (auto& layer : requestedInstanceLayers) {
            bool found = false;
            for (auto& supportedLayer : supportedLayers) {
                if (strcmp (supportedLayer.layerName, layer) == 0) {
                    found = true;
                    break;
                }
            }

            RETURN_LOG_ERROR_IF (!found, -1, "Layer not present");
        }

        VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = NULL,
            .pApplicationName = config.app_name.c_str (),
            .applicationVersion = VK_MAKE_VERSION (config.app_version.major,
                    config.app_version.minor, config.app_version.major),
            .pEngineName = config.engine_name.c_str (),
            .engineVersion = VK_MAKE_VERSION (config.engine_version.major,
                    config.engine_version.minor, config.engine_version.patch),
            .apiVersion = VK_API_VERSION_1_0
        };
        VkInstanceCreateInfo info = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = NULL,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = (uint32_t)requestedInstanceLayers.size (),
            .ppEnabledLayerNames = requestedInstanceLayers.data (),
            .enabledExtensionCount = (uint32_t)requestedInstanceExtensions.size (),
            .ppEnabledExtensionNames = requestedInstanceExtensions.data ()
        };

        VK_WRAP_RETURN_LOGERROR (vkCreateInstance (&info, nullptr, &vkapi->instance), -1,
                "Failed to create VkInstance");

#if SLRD_VULKAN_DEBUG_MESSENGER_ENABLED
        pfns.vkCreateDebugUtilsMessengerEXT = 
            (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance,
                    "vkCreateDebugUtilsMessengerEXT");
        RETURN_LOG_ERROR_IF (!pfns.vkCreateDebugUtilsMessengerEXT,
                VK_ERROR_EXTENSION_NOT_PRESENT,
                "failed to get vkCreateDebugUtilsMessengerEXT function");

        pfns.vkDestroyDebugUtilsMessengerEXT = 
            (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr (instance,
                    "vkDestroyDebugUtilsMessengerEXT");
        RETURN_LOG_ERROR_IF (!pfns.vkDestroyDebugUtilsMessengerEXT, 
                VK_ERROR_EXTENSION_NOT_PRESENT,
                "failed to get vkDestroyDebugUtilsMessengerEXT function");
#endif
        return 0;
    }

    VKAPI::~VKAPI () {
        vkDestroyInstance (instance, nullptr);
    }
};

namespace slrd::platform::vulkan {
    /* Get the data for initialized vulkan api */
    const VulkanData *getVulkanAPIData () {
        return vkapi.get ();
    }

    VkDevice getLogicalDevice (const std::shared_ptr<IDevice>& device) {
        auto vkdevice = static_cast<VKDevice *> (device.get ());
        return vkdevice->getVkDevice ();
    }

    VkPhysicalDevice getPhysicalDevice (const std::shared_ptr<IDevice>& device) {
        auto vkdevice = static_cast<VKDevice *> (device.get ());
        return vkdevice->getPhysicalDevice ();
    }

    VkQueue getQueue (const std::shared_ptr<ICommandQueue>& queue) {
        auto vkqueue = static_cast<VKCommandQueue *> (queue.get ());
        return vkqueue->getCommandQueue ();
    }

    uint32_t getQueueFamily (const std::shared_ptr<ICommandQueue>& queue) {
        auto vkqueue = static_cast<VKCommandQueue *> (queue.get ());
        return vkqueue->getCommandQueueFamily ();
    }

    VkCommandBuffer getCommandBuffer (
            const std::shared_ptr<ICommandBuffer>& commandBuffer) {
        auto vkbuffer = static_cast<VKCommandBuffer *> (commandBuffer.get ());
        return vkbuffer->getCommandBuffer ();
    }

    VkRenderPass getRenderPass (
            const std::shared_ptr<IRenderPass>& renderPass) {
        auto vkbuffer = static_cast<VKRenderPass *> (renderPass.get ());
        return vkbuffer->getRenderPass ();
    }

    VkImage getTexture (const std::shared_ptr<ITexture>& texture) {
        auto vktexture = static_cast<VKTexture *> (texture.get ());
        return vktexture->getImage ();
    }

    VkImageView getTextureView (const std::shared_ptr<ITextureView>& textureView) {
        auto vkview = static_cast<VKTextureView *> (textureView.get ());
        return vkview->getView ();
    }
    VkSampler getSampler (const std::shared_ptr<ISampler>& sampler) {
        auto vksampler = static_cast<VKSampler *> (sampler.get ());
        return vksampler->getSampler ();
    }

    VkSwapchainKHR getSwapchain (const std::shared_ptr<ISwapchain>& swapchain) {
        auto vkswapchain = static_cast<VKSwapchain *> (swapchain.get ());
        return vkswapchain->getSwapchain ();
    }
}
