/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "pipelinemanager.hpp"
#include <mutex>
#include <xxhash.h>
#include "shader.hpp"
#include "pipeline.hpp"
#include "format.hpp"
#include "vulkan/error.hpp"
#include "pipelinelayout.hpp"

namespace slrd {
    VKPipelineState::VKPipelineState (const GraphicsPipelineInfo& info) {
        const auto& ibindings = info.vertexConfig.vertexBindings;
        const auto& iattributes = info.vertexConfig.attributeDescs;

        const auto& iblendAttachments = info.colorBlendConfig.attachments;
        
        bindings.resize (ibindings.size ());
        attributes.resize (iattributes.size ());
        blendAttachments.resize (iblendAttachments.size ());

        /* msInfo */
        {
            msInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            msInfo.rasterizationSamples = slrd::getVkSampleCount (info.multisampleConfig.sampleCount);
            msInfo.alphaToCoverageEnable = info.multisampleConfig.alphaToOneEnabled;
            msInfo.sampleShadingEnable = info.multisampleConfig.sampleShadingEnabled;
        }

        /* vsInfo */
        {
            for (unsigned i = 0; i < ibindings.size (); ++i) {
                bindings[i].binding = ibindings[i].binding;
                bindings[i].stride = ibindings[i].stride;
                bindings[i].inputRate = slrd::getVkVertexInputRate (ibindings[i].inputRate);
            }

            for (unsigned i = 0; i < iattributes.size (); ++i) {
                attributes[i].binding = iattributes[i].binding;
                attributes[i].format  = slrd::getVkFormat (iattributes[i].format);
                attributes[i].offset  = iattributes[i].offset;
                attributes[i].location = iattributes[i].location;
            }
        }

        /* dpsInfo */
        {
            /* TODO: Make this a thing */
            dpsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            dpsInfo.depthTestEnable = info.depthStencilConfig.depthTestEnabled;
            dpsInfo.depthWriteEnable = info.depthStencilConfig.depthWriteEnabled;
            dpsInfo.depthCompareOp = getVkCompareOp (info.depthStencilConfig.compareOperator);
            dpsInfo.minDepthBounds = 0.f;
            dpsInfo.maxDepthBounds = 1.f;
        }

        /* blendInfo */
        {
            /* TODO: Make this a thing */
            blendInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            blendInfo.logicOpEnable = VK_FALSE;

            for (unsigned i = 0; i < iblendAttachments.size (); ++i) {
                blendAttachments[i].blendEnable  = iblendAttachments[i].blendEnabled;
                blendAttachments[i].alphaBlendOp = slrd::getVkBlendOp (iblendAttachments[i].alphaBlendOperation);
                blendAttachments[i].colorBlendOp = slrd::getVkBlendOp (iblendAttachments[i].colorBlendOperation);
                blendAttachments[i].dstAlphaBlendFactor = slrd::getVkBlendFactor (iblendAttachments[i].dstAlphaBlendFactor);
                blendAttachments[i].srcAlphaBlendFactor = slrd::getVkBlendFactor (iblendAttachments[i].srcAlphaBlendFactor);
                blendAttachments[i].dstColorBlendFactor = slrd::getVkBlendFactor (iblendAttachments[i].dstColorBlendFactor);
                blendAttachments[i].srcColorBlendFactor = slrd::getVkBlendFactor (iblendAttachments[i].srcColorBlendFactor);

                blendAttachments[i].colorWriteMask = slrd::getVkColorComponents (iblendAttachments[i].colorWriteMask);
            }

            blendInfo.pAttachments = blendAttachments.data ();
            blendInfo.attachmentCount = blendAttachments.size ();
        }

        /* rasterInfo */
        {
            const auto& rasterConfig = info.rasterizerConfig;

            /* TODO: Finish making this */
            rasterInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterInfo.cullMode = slrd::getVkCullMode (rasterConfig.cullMode);
            rasterInfo.frontFace = slrd::getVkFrontFace (rasterConfig.windingOrder);
            rasterInfo.lineWidth = 1.0f;
            rasterInfo.polygonMode = slrd::getVkPolygonMode (rasterConfig.polygonMode);
            rasterInfo.depthClampEnable = rasterConfig.depthClampEnable;
        }

        /* iaInfo */
        {
            iaInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
            iaInfo.topology = slrd::getVkPrimitiveTopology (info.inputAssembly.topology);
            iaInfo.primitiveRestartEnable = info.inputAssembly.restart;
        }

        shader = Ref<IShader>::share (info.shader);

        XXH64_state_t *state = XXH64_createState ();
        SLRD_DEBUG_CRIT_IF (!state,
                "Failed to create a state for the hashing function");
        SLRD_DEBUG_CRIT_IF (XXH64_reset (state, 0),
                "Failed to create a state for the hashing function");

        XXH64_update (state, &msInfo, sizeof (msInfo));
        XXH64_update (state, &dpsInfo, sizeof (dpsInfo));
        XXH64_update (state, &blendInfo, sizeof (blendInfo));
        XXH64_update (state, &rasterInfo, sizeof (rasterInfo));
        XXH64_update (state, &iaInfo, sizeof (iaInfo));

        XXH64_update (state, bindings.data (),
                bindings.size () * sizeof (decltype (bindings)::value_type));
        XXH64_update (state, attributes.data (),
                attributes.size () * sizeof (decltype (attributes)::value_type));
        XXH64_update (state, blendAttachments.data (),
                blendAttachments.size () *
                sizeof (decltype (blendAttachments)::value_type));

        auto *vkshader = getShader ();
        /* FIXME */
        XXH64_update (state, vkshader->getStages ().data (), vkshader->getStages ().size () *
                sizeof (VkPipelineShaderStageCreateInfo));

        hash = XXH64_digest (state);
        XXH64_freeState (state);
    }

    VKShader *VKPipelineState::getShader () const noexcept {
        return static_cast<VKShader *> (shader.get ());
    }

    void PipelineLayoutInfo::calculateHash () {
        XXH64_state_t *state = XXH64_createState ();
        SLRD_DEBUG_CRIT_IF (!state,
                "Failed to create a state for the hashing function");
        SLRD_DEBUG_CRIT_IF (XXH64_reset (state, 0),
                "Failed to create a state for the hashing function");

        XXH64_update (state, sets.data (), sets.size () *
                sizeof (decltype (sets)::value_type));
        XXH64_update (state, &pushConstants, sizeof (pushConstants));

        hash = XXH64_digest (state);
        XXH64_freeState (state);
    }


    VkPipeline PipelineManager::createPipelineForRenderPass (const VKPipelineState& state,
            VKRenderPass *rp) {
        SLRD_ASSERT (rp != nullptr);

        VkPipeline vkpipeline;

        static VkDynamicState dynamicStates[] = {
            VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT
        };

        VkPipelineVertexInputStateCreateInfo vsInfo {};
        VkPipelineDynamicStateCreateInfo dynInfo {};
        VkPipelineViewportStateCreateInfo vpInfo {};

        /* dynInfo */
        {
            dynInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynInfo.pDynamicStates = dynamicStates;
            dynInfo.dynamicStateCount = sizeof (dynamicStates) / sizeof (dynamicStates[0]);
        };

        /* vsInfo */
        {
            vsInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
            vsInfo.pVertexBindingDescriptions = state.bindings.data ();
            vsInfo.vertexBindingDescriptionCount = state.bindings.size ();

            vsInfo.pVertexAttributeDescriptions = state.attributes.data ();
            vsInfo.vertexAttributeDescriptionCount = state.attributes.size ();
        }


        /* vpInfo */
        {
            vpInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
            vpInfo.viewportCount = 1;
            vpInfo.scissorCount = 1;
        }

        auto *shader = state.getShader ();
        auto& stages = shader->getStages ();

        VkGraphicsPipelineCreateInfo plInfo {};
        plInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        plInfo.flags = 0;
        plInfo.layout = shader->grabOrCreatePipelineLayout ()->getLayout ();
        plInfo.subpass = 0;
        plInfo.pStages = stages.data ();
        plInfo.stageCount = stages.size ();
        plInfo.renderPass = rp->getRenderPass ();
        plInfo.pDynamicState = &dynInfo;
        plInfo.pMultisampleState = &state.msInfo;
        plInfo.pVertexInputState = &vsInfo;
        plInfo.pDepthStencilState = &state.dpsInfo;
        plInfo.pColorBlendState = &state.blendInfo;
        plInfo.pRasterizationState = &state.rasterInfo;
        plInfo.pViewportState = &vpInfo;
        plInfo.pInputAssemblyState = &state.iaInfo;

        VK_WRAP_RETURN_LOGERROR (
                vkCreateGraphicsPipelines (m_device->getVkDevice (), VK_NULL_HANDLE, 1, &plInfo, nullptr, &vkpipeline),
                VK_NULL_HANDLE,
                "Failed to create VkPipeline"
                );

        return vkpipeline;
    }

    VkPipeline PipelineManager::getOrCreatePipeline (const VKPipelineState& state,
                VKRenderPass *rp) {
        SLRD_ASSERT (rp != nullptr);

        std::lock_guard lock (m_pipelineMtx);

        PipelineStateHash combined[2] = {
            state.hash,
            rp->getHash ()
        };

        auto hash = XXH64 (combined, sizeof (combined) / sizeof (combined[0]), 0);

        if (auto it = m_stateRpToPipeline.find (hash);
                it != m_stateRpToPipeline.end ()) {
            return it->second;
        } else {
            return m_stateRpToPipeline.emplace (hash, createPipelineForRenderPass (state, rp)).first->second;
        }
    }

    void PipelineManager::addPipelineLayoutInfo (const PipelineLayoutInfo& info) {
        std::lock_guard lock (m_pipelineLayoutMtx);

        if (auto it = m_hashToPipelineLayoutInfo.find (info.hash);
                it != m_hashToPipelineLayoutInfo.end ()) {
            it->second.second++;
            return;
        }

        m_hashToPipelineLayoutInfo.emplace (info.hash, std::make_pair (info, 1));
    }

    VKPipelineLayout *PipelineManager::grabOrCreatePipelineLayout (PipelineLayoutInfoHash hash) {
        std::lock_guard lock (m_pipelineLayoutMtx);

        SLRD_ASSERT (m_hashToPipelineLayoutInfo.contains (hash));

        auto&[info, counter] = m_hashToPipelineLayoutInfo.at (hash);
        if (auto it = m_hashToPipelineLayout.find (hash);
                it != m_hashToPipelineLayout.end ()) {
            ++counter;
            return it->second.get ();
        }


        auto layout = std::make_unique<VKPipelineLayout> ();
        if (layout->init (m_device, info) != 0) {
            return nullptr;
        }

        ++counter;
        m_hashToPipelineLayout.emplace (info.hash, std::move (layout));
        return m_hashToPipelineLayout[hash].get ();
    }

    /* Get the pipeline layout */
    VKPipelineLayout *PipelineManager::getOrCreatePipelineLayout (PipelineLayoutInfoHash hash) {
        std::lock_guard lock (m_pipelineLayoutMtx);

        SLRD_ASSERT (m_hashToPipelineLayoutInfo.contains (hash));

        auto info = m_hashToPipelineLayoutInfo.at (hash).first;
        if (auto it = m_hashToPipelineLayout.find (hash);
                it != m_hashToPipelineLayout.end ()) {
            return it->second.get ();
        }


        auto layout = std::make_unique<VKPipelineLayout> ();
        if (layout->init (m_device, info) != 0) {
            return nullptr;
        }

        m_hashToPipelineLayout.emplace (info.hash, std::move (layout));
        return m_hashToPipelineLayout[hash].get ();
    }

    void PipelineManager::releasePipelineLayout (PipelineLayoutInfoHash hash) {
        std::lock_guard lock (m_pipelineLayoutMtx);

        SLRD_ASSERT (m_hashToPipelineLayoutInfo.contains (hash));

        auto count = --m_hashToPipelineLayoutInfo[hash].second;
        if (!count) {
            m_hashToPipelineLayoutInfo.erase (hash);
            m_hashToPipelineLayout.erase (hash);
        }
    }

    PipelineManager::PipelineManager (VKDevice *device) :
            m_device (device) {
        SLRD_ASSERT (device);
    }
    PipelineManager::~PipelineManager () {
        for (auto&[_, pipeline] : m_stateRpToPipeline) {
            vkDestroyPipeline (m_device->getVkDevice (), pipeline, nullptr);
        }
    }
}
