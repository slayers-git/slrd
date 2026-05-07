/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_PIPELINE_MANAGER_HPP__
#define __SLRD_VULKAN_PIPELINE_MANAGER_HPP__

#include <slrd/shader.hpp>
#include "slrd/pipeline.hpp"
#include "vulkan/renderpass.hpp"
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>
#include <cstring>

#include <atomic>

namespace slrd {
    class VKRenderPass;
    class VKPipelineLayout;
    class VKShader;

    using PipelineStateHash = uint64_t;

    using PipelineLayoutInfoHash = uint64_t;
    struct DescriptorSet {
        uint32_t set {};
        std::vector<VkDescriptorSetLayoutBinding> bindings;

        bool operator== (const DescriptorSet& other) const {
            return other.bindings.size () == bindings.size () &&
                memcmp (other.bindings.data (), bindings.data (),
                    bindings.size () * sizeof (VkDescriptorSetLayoutBinding));
        }

        bool operator< (const DescriptorSet& other) const {
            return set < other.set;
        }

        bool operator!= (const DescriptorSet& other) const {
            return !(*this == other);
        }
    };

    struct PipelineLayoutInfo {
        PipelineLayoutInfoHash hash;

        /* descriptor sets */
        std::vector<DescriptorSet> sets;
        /* push constant ranges for the shader */
        VkPushConstantRange pushConstants {};

        void calculateHash ();
    };

    struct VKPipelineState {
        PipelineStateHash hash {};

        Ref<IShader> shader {};

        VkPipelineMultisampleStateCreateInfo msInfo {};
        VkPipelineDepthStencilStateCreateInfo dpsInfo {};
        VkPipelineColorBlendStateCreateInfo blendInfo {};
        VkPipelineRasterizationStateCreateInfo rasterInfo {};
        VkPipelineInputAssemblyStateCreateInfo iaInfo {};

        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkPipelineColorBlendAttachmentState> blendAttachments;
        
        VKPipelineState () = default;
        VKPipelineState (const VKPipelineState&) = default;
        VKPipelineState (VKPipelineState&&) = default;
        VKPipelineState& operator= (const VKPipelineState&) = default;
        VKPipelineState& operator= (VKPipelineState&&) = default;

        VKPipelineState (const GraphicsPipelineInfo &info);
        VKPipelineState (const ComputePipelineInfo& info);

        /* Calculate combined hash for the RenderPass */
        PipelineStateHash hashForRenderPass (const VKRenderPass *rp) const;

        VKShader *getShader () const noexcept;
    };

    /* This class is responsible for managing the actual pipelines created
     * from a combination of the state and a renderpass in use */
    class PipelineManager {
    private:
        VKDevice *m_device;

        std::mutex m_pipelineMtx;
        std::mutex m_pipelineLayoutMtx;

        std::unordered_map<PipelineStateHash, VkPipeline>
            m_stateRpToPipeline;

        /* Stores the PipelineLayout itself along with the reference counting */
        std::unordered_map<PipelineLayoutInfoHash,
            std::pair<PipelineLayoutInfo, std::atomic<uint32_t>>> m_hashToPipelineLayoutInfo;
        std::unordered_map<PipelineLayoutInfoHash,
            std::unique_ptr<VKPipelineLayout>> m_hashToPipelineLayout;

        /* TODO memory management */

        VkPipeline createPipelineForRenderPass (const VKPipelineState& state,
                VKRenderPass *rp);

    public:
        PipelineManager (VKDevice *device);

        ~PipelineManager();

        VkPipeline getOrCreatePipeline (const VKPipelineState& state,
                VKRenderPass *rp);

        void addPipelineLayoutInfo (const PipelineLayoutInfo& info);
        /* Get the pipeline layout */
        VKPipelineLayout *getOrCreatePipelineLayout (PipelineLayoutInfoHash hash);

        VKPipelineLayout *grabOrCreatePipelineLayout (PipelineLayoutInfoHash hash);
        /* Decrement the reference counter for the pipeline layout */
        void releasePipelineLayout (PipelineLayoutInfoHash hash);
    };
};

#endif /* #define __SLRD_VULKAN_PIPELINE_MANAGER_HPP__ */
