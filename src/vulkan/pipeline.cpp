/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "pipeline.hpp"
#include "pipelinelayout.hpp"
#include "shader.hpp"
#include "renderpass.hpp"
#include "vulkan/error.hpp"
#include <vulkan/vulkan_core.h>
#include "uniformset.hpp"

namespace slrd {
    VKPipeline::~VKPipeline () {
        if (m_pipeline)
            vkDestroyPipeline (m_device->getVkDevice (), m_pipeline, nullptr);
    }

    int VKPipeline::init (VKDevice *device, const GraphicsPipelineInfo& info) {
        RETURN_LOG_ERROR_IF (!info.shader,
                -1,
                "Shader were not specified in the pipeline creation info");

        m_state = VKPipelineState (info);

        setParentDevice (device);
        m_bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

        return 0;
    }

    int VKPipeline::init (VKDevice *device, const ComputePipelineInfo& info) {
        VkPipeline vkpipeline;

        RETURN_LOG_ERROR_IF (!info.shader,
                -1,
                "Shader were not specified in the pipeline creation info");

        auto iShader = static_cast<VKShader *> (info.shader);
        const std::vector<VkPipelineShaderStageCreateInfo>& stages = iShader->getStages ();

        SLRD_COMPLAIN_RETURN (stages.size () != 1 ||
                stages[0].stage != VK_SHADER_STAGE_COMPUTE_BIT, -1,
                "ComputePipeline requires exactly one [compute] shader!");

        VkComputePipelineCreateInfo cpInfo {};
        cpInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        cpInfo.stage = stages[0];
        cpInfo.layout = iShader->getOrCreatePipelineLayout ()->getLayout ();

        VK_WRAP_RETURN_RESULT_LOGERROR (
                vkCreateComputePipelines (device->getVkDevice (), VK_NULL_HANDLE, 1,
                    &cpInfo, nullptr, &vkpipeline),
                "Failed to create VkPipeline (compute)"
                );

        setParentDevice (device);
        m_pipeline = vkpipeline;
        m_bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;
        m_state.shader = Ref<IShader>::share (info.shader);

        return 0;
    }

    VKPipelineLayout *VKPipeline::getPipelineLayout () {
        SLRD_ASSERT (m_state.shader != nullptr);
        return m_state.getShader ()->getOrCreatePipelineLayout ();
    }

    Ref<IUniformSet> VKPipeline::allocateUniformSet (uint32_t set) {
        SLRD_ASSERT (m_state.shader);
        return m_state.getShader ()->allocateUniformSet (set);
    }
};
