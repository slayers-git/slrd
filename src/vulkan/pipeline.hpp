/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_PIPELINE_HPP__
#define __SLRD_VULKAN_PIPELINE_HPP__

#include "api.hpp"
#include "debug.hpp"
#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include "pipelinemanager.hpp"

namespace slrd {
    class VKDevice;
    class VKRenderPass;
    class VKPipelineLayout;

    inline constexpr VkVertexInputRate getVkVertexInputRate (slrd::VertexInputRate rate) {
        switch (rate) {
            case VERTEX_INPUT_RATE_VERTEX:
                return VK_VERTEX_INPUT_RATE_VERTEX;
            case VERTEX_INPUT_RATE_INSTANCE:
                return VK_VERTEX_INPUT_RATE_INSTANCE;
        }

        SLRD_DEBUG_CRIT ("getVkVertexInputRate: invalid enum");
    }

    inline constexpr VkFrontFace getVkFrontFace (slrd::WindingOrder order) {
        switch (order) {
            case WINDING_ORDER_CLOCKWISE:
                return VK_FRONT_FACE_CLOCKWISE;
            case WINDING_ORDER_COUNTERCLOCKWISE:
                return VK_FRONT_FACE_COUNTER_CLOCKWISE;
        }

        SLRD_DEBUG_CRIT ("getVkFrontFace: invalid enum");
    }

    inline constexpr VkCullModeFlags getVkCullMode (slrd::CullMode mode) {
        switch (mode) {
            case CULL_MODE_BACK:
                return VK_CULL_MODE_BACK_BIT;
            case CULL_MODE_FRONT:
                return VK_CULL_MODE_FRONT_BIT;
            case CULL_MODE_NONE:
                return VK_CULL_MODE_NONE;
        }

        SLRD_DEBUG_CRIT ("getVkCullMode: invalid enum");
    }

    inline constexpr VkPolygonMode getVkPolygonMode (slrd::PolygonMode mode) {
        switch (mode) {
            case POLYGON_MODE_FILL:
                return VK_POLYGON_MODE_FILL;
            case POLYGON_MODE_LINE:
                return VK_POLYGON_MODE_LINE;
            case POLYGON_MODE_POINT:
                return VK_POLYGON_MODE_POINT;
        }

        SLRD_DEBUG_CRIT ("getVkPolygonMode: invalid enum");
    }

    inline constexpr VkPrimitiveTopology getVkPrimitiveTopology (slrd::PrimitiveTopology topology) {
#define __TOPO_CASE(__Type) \
        case PRIMITIVE_TOPOLOGY_ ## __Type: topo = VK_PRIMITIVE_TOPOLOGY_ ## __Type; break;

        VkPrimitiveTopology topo = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

        switch (topology) {
            __TOPO_CASE (POINT_LIST);
            __TOPO_CASE (LINE_LIST);
            __TOPO_CASE (LINE_STRIP);
            __TOPO_CASE (TRIANGLE_LIST);
            __TOPO_CASE (TRIANGLE_STRIP);
            __TOPO_CASE (TRIANGLE_FAN);
            __TOPO_CASE (PATCH_LIST);

            case PRIMITIVE_TOPOLOGY_LINE_LIST_ADJACENCY:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
            case PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJACENCY:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJACENCY:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
            case PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJACENCY:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;

            default:
                SLRD_DEBUG_CRIT ("getVkPrimitiveTopology: invalid enum");
                break;
        }

#undef __TOPO_CASE
        return topo;
    }

    inline constexpr VkBlendOp getVkBlendOp (slrd::BlendOperation operation) {
#define __BLEND_CASE(__Op) \
        case BLEND_OPERATION_ ## __Op: blend = VK_BLEND_OP_ ## __Op; break;

        VkBlendOp blend = VK_BLEND_OP_ADD;

        switch (operation) {
            __BLEND_CASE (ADD);
            __BLEND_CASE (SUBTRACT);
            __BLEND_CASE (MIN);
            __BLEND_CASE (MAX);
            __BLEND_CASE (REVERSE_SUBTRACT);

            default:
                SLRD_DEBUG_CRIT ("getVkBlendOp: invalid enum");
                break;
        }

#undef __BLEND_CASE
        return blend;
    }
    inline constexpr VkBlendFactor getVkBlendFactor (slrd::BlendFactor operation) {
#define __BLEND_CASE(__Op) \
        case BLEND_FACTOR_ ## __Op: blend = VK_BLEND_FACTOR_ ## __Op; break;

        VkBlendFactor blend = VK_BLEND_FACTOR_ONE;

        switch (operation) {
            __BLEND_CASE (ONE);
            __BLEND_CASE (ZERO);
            __BLEND_CASE (SRC_COLOR);
            __BLEND_CASE (ONE_MINUS_SRC_COLOR);
            __BLEND_CASE (DST_COLOR);
            __BLEND_CASE (ONE_MINUS_DST_COLOR);
            __BLEND_CASE (SRC_ALPHA);
            __BLEND_CASE (ONE_MINUS_SRC_ALPHA);
            __BLEND_CASE (DST_ALPHA);
            __BLEND_CASE (ONE_MINUS_DST_ALPHA);
            __BLEND_CASE (CONSTANT_COLOR);
            __BLEND_CASE (ONE_MINUS_CONSTANT_COLOR);
            __BLEND_CASE (CONSTANT_ALPHA);
            __BLEND_CASE (ONE_MINUS_CONSTANT_ALPHA);
            __BLEND_CASE (SRC_ALPHA_SATURATE);
            __BLEND_CASE (SRC1_COLOR);
            __BLEND_CASE (ONE_MINUS_SRC1_COLOR);
            __BLEND_CASE (SRC1_ALPHA);
            __BLEND_CASE (ONE_MINUS_SRC1_ALPHA);

            default:
                SLRD_DEBUG_CRIT ("getVkBlendFactor: invalid enum");
                break;
        }

#undef __BLEND_CASE
        return blend;
    }
    
    inline constexpr VkColorComponentFlags getVkColorComponents (slrd::ColorWriteMask mask) {
        VkColorComponentFlags comps = 0;

        if (mask & COLOR_MASK_RED)
            comps |= VK_COLOR_COMPONENT_R_BIT;
        if (mask & COLOR_MASK_GREEN)
            comps |= VK_COLOR_COMPONENT_G_BIT;
        if (mask & COLOR_MASK_BLUE)
            comps |= VK_COLOR_COMPONENT_B_BIT;
        if (mask & COLOR_MASK_ALPHA)
            comps |= VK_COLOR_COMPONENT_A_BIT;

        return comps;
    }


    SLRD_RESOURCE_DEFINE_TYPE(VKPipeline);
    class VKPipeline :
            public VKDeviceObject<IPipeline>,
            public VKResource<VKPipeline> {
    private:
        VkPipelineBindPoint m_bindPoint;
        /* If the pipeline is compute, then this variable will be used, since 
         * we don't have to provide a renderpass. Otherwise the pipeline object
         * will be stored inside of PipelineManager. */
        VkPipeline m_pipeline = VK_NULL_HANDLE;

        /* From which pool to allocate descriptor sets */
        VkDescriptorPool m_pool;

        /* The state in case the pipeline for the render pass doesn't exist yet */
        VKPipelineState m_state;

    public:
        VKPipeline () {};
        ~VKPipeline ();

        [[nodiscard]] VkPipeline getPipeline () const {
            return m_pipeline;
        }

        [[nodiscard]] const auto& getState () const {
            return m_state;
        }

        [[nodiscard]] const auto& getBindPoint () const {
            return m_bindPoint;
        }

        int init (VKDevice *device, const GraphicsPipelineInfo& info);
        int init (VKDevice *device, const ComputePipelineInfo& info);

        VKPipelineLayout *getPipelineLayout ();

        Ref<IUniformSet> allocateUniformSet (uint32_t set) final;
    };

    inline VKPipeline *createVKGraphicsPipeline (VKDevice *device,
            const GraphicsPipelineInfo& info) {
        return makeResource<VKPipeline> (device, info);
    }
    /* I have no idea why I made them separate functions */
    inline VKPipeline *createVKComputePipeline (VKDevice *device,
            const ComputePipelineInfo& info) {
        return makeResource<VKPipeline> (device, info);
    }

    inline constexpr VkCompareOp getVkCompareOp (CompareOperator compare) {
#define __COMPARE_CASE(__Op) \
        case COMPARE_OPERATOR_ ## __Op: op = VK_COMPARE_OP_ ## __Op; break;

        VkCompareOp op = VK_COMPARE_OP_LESS;

        switch (compare) {
            __COMPARE_CASE (NEVER);
            __COMPARE_CASE (LESS);
            __COMPARE_CASE (EQUAL);
            __COMPARE_CASE (LESS_OR_EQUAL);
            __COMPARE_CASE (GREATER);
            __COMPARE_CASE (NOT_EQUAL);
            __COMPARE_CASE (GREATER_OR_EQUAL);
            __COMPARE_CASE (ALWAYS);

            default:
                SLRD_DEBUG_CRIT ("getVkBlendFactor: invalid enum");
                break;
        }

#undef __COMPARE_CASE
        return op;
    }
};

#endif /* #define __SLRD_VULKAN_PIPELINE_HPP__ */
