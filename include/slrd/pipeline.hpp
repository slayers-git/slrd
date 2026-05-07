/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_PIPELINE_HPP__
#define __SLRD_PIPELINE_HPP__

#include "object.hpp"

#include "buffer.hpp"
#include "format.hpp"
#include <cstdint>
#include <memory>
#include <span>

namespace slrd {
    class IUniformSet;
    class IShader;

    enum PolygonMode {
        POLYGON_MODE_FILL,
        POLYGON_MODE_LINE,
        POLYGON_MODE_POINT
    };

	enum StencilOperation {
		STENCIL_OPERATION_KEEP = 0,
		STENCIL_OPERATION_ZERO = 1,
		STENCIL_OPERATION_REPLACE = 2,
		STENCIL_OPERATION_INCREMENT_AND_CLAMP = 3,
		STENCIL_OPERATION_DECREMENT_AND_CLAMP = 4,
		STENCIL_OPERATION_INVERT = 5,
		STENCIL_OPERATION_INCREMENT_AND_WRAP = 6,
		STENCIL_OPERATION_DECREMENT_AND_WRAP = 7,
	};

    enum CompareOperator {
        COMPARE_OPERATOR_NEVER,
        COMPARE_OPERATOR_LESS,
        COMPARE_OPERATOR_EQUAL,
        COMPARE_OPERATOR_LESS_OR_EQUAL,
        COMPARE_OPERATOR_GREATER,
        COMPARE_OPERATOR_NOT_EQUAL,
        COMPARE_OPERATOR_GREATER_OR_EQUAL,
        COMPARE_OPERATOR_ALWAYS,
    };

	enum VertexInputRate {
        VERTEX_INPUT_RATE_VERTEX,
        VERTEX_INPUT_RATE_INSTANCE
	};

	enum ColorWriteMask {
		COLOR_MASK_RED = 0B0001,
		COLOR_MASK_GREEN = 0B0010,
		COLOR_MASK_BLUE = 0B0100,
		COLOR_MASK_ALPHA = 0B1000,
		COLOR_MASK_RGB = COLOR_MASK_RED | COLOR_MASK_GREEN | COLOR_MASK_BLUE,
		COLOR_MASK_RGBA = COLOR_MASK_RGB | COLOR_MASK_ALPHA
	};

	enum BindingType {
        BINDING_TYPE_COMBINED_TEXTURE_SAMPLER = 0,
        BINDING_TYPE_TEXTURE,
		BINDING_TYPE_SAMPLER, 
        BINDING_TYPE_UNIFORM_BUFFER,
        BINDING_TYPE_STORAGE_BUFFER, 
        BINDING_TYPE_UNIFORM_BUFFER_DYNAMIC, 
        BINDING_TYPE_STORAGE_BUFFER_DYNAMIC, 
	};

    #define BINDING_TYPE_MAX_ENUM (BINDING_TYPE_STORAGE_BUFFER_DYNAMIC + 1)

	enum PrimitiveTopology {
		PRIMITIVE_TOPOLOGY_POINT_LIST,
        PRIMITIVE_TOPOLOGY_LINE_LIST,
        PRIMITIVE_TOPOLOGY_LINE_STRIP,
        PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP,
        PRIMITIVE_TOPOLOGY_TRIANGLE_FAN,
		PRIMITIVE_TOPOLOGY_LINE_LIST_ADJACENCY,
        PRIMITIVE_TOPOLOGY_LINE_STRIP_ADJACENCY,
        PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_ADJACENCY,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_ADJACENCY,
		PRIMITIVE_TOPOLOGY_PATCH_LIST
	};

    enum BlendFactor {
        BLEND_FACTOR_ZERO = 0,
        BLEND_FACTOR_ONE = 1,
        BLEND_FACTOR_SRC_COLOR = 2,
        BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
        BLEND_FACTOR_DST_COLOR = 4,
        BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
        BLEND_FACTOR_SRC_ALPHA = 6,
        BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
        BLEND_FACTOR_DST_ALPHA = 8,
        BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
        BLEND_FACTOR_CONSTANT_COLOR = 10,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR = 11,
        BLEND_FACTOR_CONSTANT_ALPHA = 12,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        BLEND_FACTOR_SRC_ALPHA_SATURATE,
        BLEND_FACTOR_SRC1_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        BLEND_FACTOR_SRC1_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA
    };

    enum BlendOperation : uint8_t {
        BLEND_OPERATION_ADD,
        BLEND_OPERATION_SUBTRACT,
        BLEND_OPERATION_REVERSE_SUBTRACT,
        BLEND_OPERATION_MIN,
        BLEND_OPERATION_MAX
    };

    enum WindingOrder : uint8_t {
        WINDING_ORDER_CLOCKWISE,
        WINDING_ORDER_COUNTERCLOCKWISE
    };

    enum CullMode : uint8_t {
        CULL_MODE_NONE  = 0,
        CULL_MODE_FRONT = 1,
        CULL_MODE_BACK  = 2,
    };

    struct VertexBindingDescription {
        uint32_t binding;
        uint32_t stride;
        VertexInputRate inputRate = VERTEX_INPUT_RATE_VERTEX;
    };

    struct VertexAttributeDescription {
        uint32_t location;
        uint32_t binding;
        uint32_t offset;

        Format format;
    };

    struct ColorBlendAttachment {
        BlendFactor srcColorBlendFactor = BLEND_FACTOR_ONE,
            dstColorBlendFactor = BLEND_FACTOR_ZERO,
            srcAlphaBlendFactor = BLEND_FACTOR_ONE,
            dstAlphaBlendFactor = BLEND_FACTOR_ONE;

        BlendOperation colorBlendOperation = BLEND_OPERATION_ADD,
                       alphaBlendOperation = BLEND_OPERATION_ADD;

        ColorWriteMask colorWriteMask = COLOR_MASK_RGBA;
        bool blendEnabled = false;
    };

    class IRenderPass;
	struct GraphicsPipelineInfo {
		IShader *shader = nullptr;

		struct VertexConfig {
            std::span<const VertexBindingDescription> vertexBindings;
            std::span<const VertexAttributeDescription> attributeDescs;
		} vertexConfig;

		struct InputAssemblyDesc {
			PrimitiveTopology topology = PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			bool restart = false;
		} inputAssembly;

        Viewport viewport;
        Scissor  scissor;

		struct RasterizerConfig {
			bool depthClampEnable = false;
			bool rasterizerDiscardEnable = false;

			PolygonMode polygonMode = POLYGON_MODE_FILL;
			CullMode cullMode = CULL_MODE_BACK;

			WindingOrder windingOrder = WINDING_ORDER_COUNTERCLOCKWISE;

			struct DepthBias {
				float clamp = 0;
                float constantFactor = 0;
                float slopeFactor = 0;

				bool enable = false;
			} depthBias;
		} rasterizerConfig;

		struct MultisampleConfig {
			MSAACount sampleCount = MSAACount::MSAA_COUNT_1;
			bool sampleShadingEnabled = false;	
			bool alphaToCoverageEnabled = false;
			bool alphaToOneEnabled = false;
		} multisampleConfig;

		struct ColorBlendConfig {
            std::span<const ColorBlendAttachment> attachments;
			float blendconstants[4] = { 0, 0, 0, 0 };
		} colorBlendConfig;

		struct DepthStencilConfig {
			Format depthFormat = FORMAT_UNDEFINED;
			Format stencilFormat = FORMAT_UNDEFINED;
			bool depthTestEnabled = false;
			bool depthWriteEnabled = false;
			CompareOperator compareOperator = COMPARE_OPERATOR_NEVER;

			bool stencilTestEnabled = false;
			StencilOperation stencilFrontOperation = STENCIL_OPERATION_KEEP;
			StencilOperation stencilBackOperation = STENCIL_OPERATION_KEEP;
		} depthStencilConfig;
	};

    struct ComputePipelineInfo {
        IShader *shader;
    };

    struct BindBufferInfo {
        uint32_t location;
        uint32_t set;

        IBuffer *buffer;
        uint64_t offset;
        uint64_t range;
    };

    class IPipeline : public IObject {
    public:
        virtual ~IPipeline () = default;

        /**
         * Allocate a UniformSet that is compatible with this pipeline */
        virtual Ref<IUniformSet> allocateUniformSet (uint32_t set) = 0;
    };
};

#endif /* #define __SLRD_PIPELINE_HPP__ */
