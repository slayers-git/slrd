/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_DEVICE_HPP__
#define __SLRD_VULKAN_DEVICE_HPP__

#include "slrd/pipeline.hpp"
#include "vulkan/factory.hpp"
#include <map>
#include <slrd/device.hpp>
#include <vulkan/vulkan.h>

#include <vk_mem_alloc.h>
#include "descpoolmanager.hpp"
#include "pipelinemanager.hpp"

#include <slrd/config.hpp>

#include "../refcnt.hpp"

namespace slrd {
    struct QueueIndices {
        /* We hope that present and graphics queue are the same, which is
         * almost always the case */
        uint32_t present;
        uint32_t graphics;
        /* For now we look out for the queue that is both graphics and compute
         * but in case there is ever logic needed to create a queue that is
         * compute only - there it is */
        uint32_t compute;
    };

    class VKShader;
    class VKCommandQueue;
    class VKCommandBuffer;
    class VKUniformSet;
    class VKFramebuffer;
    class VKPipeline;
    class VKBuffer;
    class VKSampler;

    /**
     * Implementation for a reference-counted Vulkan device */
    SLRD_RESOURCE_DEFINE_TYPE (VKDevice, VK_OBJECT_TYPE_DEVICE);
    class VKDevice :
        public SimpleRefCounted<IDevice>,
        public VKNamedResource<VKDevice> {
    private:
        VkDevice m_device;
        /* A reference, not created, therefore doesn't need to be deallocated */
        VkPhysicalDevice m_physicalDevice;

        VkQueue m_graphicsQueue;
        VkQueue m_presentQueue;
        VmaAllocator m_vma;

        QueueIndices m_indices;

        VkPipelineStageFlags m_pipelineShaderStages;

        /* FIXME: Add memory management for this */
        std::map<PoolKey, std::unique_ptr<DescriptorPoolManager>> m_descriptorManagers;

        /* Set layouts */
        std::map<PoolKey, VkDescriptorSetLayout> m_setLayouts;

        std::unique_ptr<PipelineManager> m_pipelineManager;

    public:
        VKDevice () = default;
        ~VKDevice ();

        [[nodiscard]] VkDevice getVkDevice () const {
            return m_device;
        }

        [[nodiscard]] VkPhysicalDevice getPhysicalDevice () const {
            return m_physicalDevice;
        }

        [[nodiscard]] VkQueue getGraphicsQueue () const {
            return m_graphicsQueue;
        }

        [[nodiscard]] VkQueue getComputeQueue () const {
            return m_graphicsQueue;
        }

        [[nodiscard]] VkQueue getPresentQueue () const {
            return m_presentQueue;
        }

        [[nodiscard]] const auto& getQueueIndices () const {
            return m_indices;
        }

        [[nodiscard]] PipelineManager *getPipelineManager () {
            return m_pipelineManager.get ();
        }

        [[nodiscard]] auto getShaderStages () const {
            return m_pipelineShaderStages;
        }

        [[nodiscard]] auto getVkAllocator () const {
            return m_vma;
        }

        DescriptorPoolManager *allocateOrGetDescriptorManager (PoolKey key);
        void clearDescriptorManagers ();

#if SLRD_VULKAN_DEBUG_MESSENGER_ENABLED
        VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif

        int init (const DeviceConfig&);

        Ref<ITexture> createTexture (const TextureInfo& info) final override;
        Ref<ISwapchain> createSwapchain (const SwapchainInfo& info) final override;
        Ref<IShader> createShader (const ShaderInfo& info) final override;

        Ref<IRenderPass> createRenderPass (const RenderPassInfo&) final override;

        Ref<IPipeline> createGraphicsPipeline (const GraphicsPipelineInfo& info) final override;
        Ref<IPipeline> createComputePipeline (const ComputePipelineInfo& info) final override;

        Ref<IBuffer> createBuffer (const BufferInfo& info) final override;
        Ref<IFence> createFence (bool signalled = false) final override;

        Ref<ICommandQueue> createCommandQueue (const CommandQueueInfo& info) final override;

        Ref<ISampler> createSampler (const SamplerInfo& info) final override;

        void waitIdle () final override;
    };

    inline IDevice *createVKDevice (const DeviceConfig& config) {
        return makeResource<VKDevice> (config);
    }
};

#endif /* #define __SLRD_VULKAN_DEVICE_HPP__ */
