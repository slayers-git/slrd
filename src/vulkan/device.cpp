/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "api.hpp"

#include <map>
#include <vector>

#include "swapchain.hpp"
#include "error.hpp"
#include "vulkan/buffer.hpp"
#include "vulkan/commandbuffer.hpp"
#include "vulkan/commandqueue.hpp"
#include "vulkan/fence.hpp"
#include "vulkan/pipeline.hpp"
#include "vulkan/renderpass.hpp"
#include "vulkan/shader.hpp"
#include "vulkan/texture.hpp"
#include "vulkan/sampler.hpp"
#include "device.hpp"

#if SLRD_VULKAN_DEBUG_MESSENGER_ENABLED

    VKAPI_CALL VkBool32 slrdDebugMessager (
            VkDebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT                  messageTypes,
            const VkDebugUtilsMessengerCallbackDataEXT*      pCallbackData,
            void*                                            pUserData) {
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            std::cerr << "!!! SLRD VULKAN LAYER ERROR !!!\n";
        }
        std::cerr << pCallbackData->pMessage << '\n';
#if SLRD_DEBUG
        if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            abort ();
        }
#endif
        return VK_FALSE;
    }

#endif

namespace slrd {
    static QueueIndices findQueueIndices (VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) {
        QueueIndices indices;

        uint32_t count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &count, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies (count);
        vkGetPhysicalDeviceQueueFamilyProperties (physicalDevice, &count,
                queueFamilies.data ());

        for (uint32_t i = 0; i < queueFamilies.size (); ++i) {
            /*VkBool32 present = false;*/
            /*vkGetPhysicalDeviceSurfaceSupportKHR (physicalDevice, i, surface, &present);*/
            /**/
            /*if (present)*/
            /*    indices.present = i;*/

            /* Vulkan spec guarantees that one such queue exists */
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
                    queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                indices.graphics = i;
                indices.present = i;
                indices.compute = i;
            }
        }

        return indices;
    }

    DescriptorPoolManager *VKDevice::allocateOrGetDescriptorManager (PoolKey key) {
        if (auto it = m_descriptorManagers.find (key);
                 it != m_descriptorManagers.end ()) {
            return it->second.get ();
        }

        auto manager = std::make_unique<DescriptorPoolManager> ();
        if (manager->init (m_device, key, 1024)) {
            return nullptr;
        }

        auto[it, _] = m_descriptorManagers.emplace (key, std::move (manager));
        return it->second.get ();
    }

    void VKDevice::clearDescriptorManagers () {
        m_descriptorManagers.clear ();
    }

    void printDevice (const VkPhysicalDeviceProperties& props, const VkPhysicalDeviceFeatures& features) {
        std::cout << std::format ("Device {} ({:x}:{:x})\n", props.deviceName, props.vendorID, props.deviceID);
    }

    int VKDevice::init (const DeviceConfig& config) {
        /* Sanity */
        if (!vkapi)
            return -1;

        /* Find a suitable device */
        uint32_t count = 0;
        RETURN_LOG_ERROR_IF (
                vkEnumeratePhysicalDevices (vkapi->instance, &count, nullptr),
                -1,
                "Failed to enumerate physical devices");
        std::vector<VkPhysicalDevice> devices (count);

        RETURN_LOG_ERROR_IF (vkEnumeratePhysicalDevices (vkapi->instance, &count, devices.data ()),
                -1,
                "Failed to enumerate physical devices");

        
        std::multimap<uint32_t, VkPhysicalDevice> ratings;
        for (uint32_t i = 0; i < devices.size (); ++i) {
            uint32_t score = 0;

            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties (devices[i], &props);

            VkPhysicalDeviceFeatures features;
            vkGetPhysicalDeviceFeatures (devices[i], &features);

            switch (props.deviceType) {
                case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                    score += 1000;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_CPU:
                    score += 100;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                    score += 500;
                    break;
                case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                    score += 900;
                    break;
                default:
                    break;
            }

            /* Skip over devices that don't have the features that we need */
            if (!features.samplerAnisotropy &&
                    !features.geometryShader) {
                continue;
            }

            ratings.emplace (score, devices[i]);
        }

        const auto it = ratings.rbegin ();
        auto bestDevice = it == ratings.rend () ? nullptr : it->second;

        RETURN_LOG_ERROR_IF (!bestDevice, -1, "Failed to find suitable physical device");

        m_physicalDevice = bestDevice;

        m_indices = findQueueIndices (m_physicalDevice, VK_NULL_HANDLE);

        const float qPrios[] = { 1.f };

        VkDeviceQueueCreateInfo qInfo[2];
        qInfo[0] = {};
        qInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qInfo[0].queueCount = 1;
        qInfo[0].queueFamilyIndex = m_indices.graphics;
        qInfo[0].pQueuePriorities = qPrios;
        qInfo[0].flags = 0;

        uint32_t queueCreateInfos = 1;

        if (m_indices.graphics != m_indices.present) {
            qInfo[1] = {};
            qInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            qInfo[1].queueCount = 1;
            qInfo[1].queueFamilyIndex = m_indices.present;
            qInfo[1].pQueuePriorities = qPrios;

            queueCreateInfos = 2;
        }

        /* Create the logical device */
        VkPhysicalDeviceFeatures enabledFeatures = {};
        enabledFeatures.samplerAnisotropy = VK_TRUE;

        VkDeviceCreateInfo dvcInfo = {};
        dvcInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        dvcInfo.pEnabledFeatures = &enabledFeatures;
        dvcInfo.enabledExtensionCount = config.device_extensions.size ();
        dvcInfo.ppEnabledExtensionNames = config.device_extensions.data ();
        dvcInfo.queueCreateInfoCount = queueCreateInfos;
        dvcInfo.pQueueCreateInfos = qInfo;

        VkResult result;
        result = vkCreateDevice (m_physicalDevice, &dvcInfo, nullptr, &m_device);
        RETURN_LOG_ERROR_IF (result != VK_SUCCESS, -1, "Failed to create VkDevice");

        /* The rest are dependent on the features */
        m_pipelineShaderStages = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
                    VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
                    VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        vkGetDeviceQueue (m_device, m_indices.present, 0, &m_presentQueue);
        vkGetDeviceQueue (m_device, m_indices.graphics, 0, &m_graphicsQueue);

        /*m_presentCommandPool = createCommandPool (m_queueFamilies.graphicsFamily);*/
        /*RETURN_LOG_ERROR_IF (!m_presentCommandPool, -1, "Failed to create the command pool for presentation");*/

        VmaAllocatorCreateInfo vmaInfo {};
        vmaInfo.physicalDevice = m_physicalDevice;
        vmaInfo.instance = vkapi->instance;
        vmaInfo.device = m_device;

        result = vmaCreateAllocator (&vmaInfo, &m_vma);
        RETURN_LOG_ERROR_IF (result != VK_SUCCESS, -1, "Failed to create VmaAllocator");

#if SLRD_VULKAN_DEBUG_MESSENGER_ENABLED
        if (config.debug && (getAPIConfig ()->debug_flags & API_DEBUG_FLAG_LAYERS)) {
            VkDebugUtilsMessengerCreateInfoEXT debugInfo {};
            debugInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debugInfo.pfnUserCallback = slrdDebugMessager;
            debugInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debugInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;

            VK_WRAP_RETURN_RESULT_LOGERROR (
                    vkapi->pfns.vkCreateDebugUtilsMessengerEXT (vkapi->instance, &debugInfo, nullptr, &m_debugMessenger),
                    "Failed to create VkDebugUitlsMessengerEXT"
                    );
        }
#endif

        m_pipelineManager = std::make_unique<PipelineManager> (this);

#ifdef SLRD_RESOURCE_PROFILER
        if (getAPIConfig ()->debug_flags & API_DEBUG_RESOURCE_PROFILER) {
            if (config.debug && config.debug_flags & DEVICE_DEBUG_FLAG_RESOURCE_PROFILER)
                m_profiler = std::make_unique<ResourceProfiler> ();

            if (config.debug && config.debug_flags & DEVICE_DEBUG_FLAG_API_RESOURCE_PROFILER)
                m_vkprofiler = std::make_unique<VKResourceProfiler> ();
        }
#endif

        return 0;
    }

    Ref<ITexture> VKDevice::createTexture (const TextureInfo& info) {
        return Ref<ITexture>::adopt (createVKTexture (this, info));
    }

    Ref<ISwapchain> VKDevice::createSwapchain (const SwapchainInfo& info) {
        return Ref<ISwapchain>::adopt (createVKSwapchain (this, info));
    }

    Ref<IShader> VKDevice::createShader (const ShaderInfo& info) {
        return Ref<IShader>::adopt (createVKShader (this, info));
    }

    Ref<IPipeline> VKDevice::createGraphicsPipeline (const GraphicsPipelineInfo& info) {
        return Ref<IPipeline>::adopt (createVKGraphicsPipeline (this, info));
    }
    Ref<IPipeline> VKDevice::createComputePipeline (const ComputePipelineInfo& info) {
        return Ref<IPipeline>::adopt (createVKComputePipeline (this, info));
    }

    Ref<IBuffer> VKDevice::createBuffer (const BufferInfo& info) {
        return Ref<IBuffer>::adopt (createVKBuffer (this, info));
    }

    Ref<IFence> VKDevice::createFence (bool signalled) {
        return Ref<IFence>::adopt (createVKFence (this, signalled));
    }

    Ref<ICommandQueue> VKDevice::createCommandQueue (const CommandQueueInfo& info) {
        return Ref<ICommandQueue>::adopt (createVKCommandQueue (this, info));
    }

    Ref<IRenderPass> VKDevice::createRenderPass (const RenderPassInfo& info) {
        return Ref<IRenderPass>::adopt (createVKRenderPass (this, info));
    }

    Ref<ISampler> VKDevice::createSampler (const SamplerInfo& info) {
        return Ref<ISampler>::adopt (createVKSampler (this, info));
    }

    void VKDevice::waitIdle () {
        vkDeviceWaitIdle (m_device);
    }

    void VKDevice::allocate (ObjectType type, DeviceSize size) noexcept {
#ifdef SLRD_RESOURCE_PROFILER
        if (m_profiler)
            ResourceProfilerWriter (*m_profiler).allocate (type, size);
#endif
    }

    void VKDevice::deallocate (ObjectType type, DeviceSize size) noexcept {
#ifdef SLRD_RESOURCE_PROFILER
        if (m_profiler)
            ResourceProfilerWriter (*m_profiler).deallocate (type, size);
#endif
    }

    void VKDevice::vkallocate (VkObjectType type, VkDeviceSize size) noexcept {
#ifdef SLRD_RESOURCE_PROFILER
        if (m_vkprofiler)
            m_vkprofiler->allocate (type, size);
#endif
    }

    void VKDevice::vkdeallocate (VkObjectType type, VkDeviceSize size) noexcept {
#ifdef SLRD_RESOURCE_PROFILER
        if (m_vkprofiler)
            m_vkprofiler->deallocate (type, size);
#endif
    }

    const ResourceProfiler *VKDevice::getResourceProfiler () const noexcept {
        return m_profiler.get ();
    }

    const VKResourceProfiler *VKDevice::getVkResourceProfiler () const noexcept {
        return m_vkprofiler.get ();
    }

    VKDevice::~VKDevice () {
        waitIdle ();

        m_pipelineManager = nullptr;
        clearDescriptorManagers ();

        vmaDestroyAllocator (m_vma);
        vkDestroyDevice (m_device, nullptr);

#if SLRD_VULKAN_DEBUG_MESSENGER_ENABLED
        if (m_debugMessenger && (getAPIConfig ()->debug_flags & API_DEBUG_FLAG_LAYERS)) {
            vkapi->pfns.vkDestroyDebugUtilsMessengerEXT (vkapi->instance, m_debugMessenger, nullptr);
        }
#endif

    }
};
