/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_SHADER_HPP__
#define __SLRD_SHADER_HPP__

#include "object.hpp"

#include "format.hpp"
#include "pipeline.hpp"
#include <memory>
#include <optional>
#include <vector>

namespace slrd {
    /* Type of the shader module */
    enum ShaderType {
        SHADER_TYPE_VERTEX,
        SHADER_TYPE_FRAGMENT,
        SHADER_TYPE_GEOMETRY,
        SHADER_TYPE_COMPUTE
    };

    /* Type of a shader struct/block member */
    enum ShaderMemberType {
        /***************************************/
        /* DO NOT CHANGE THE DEFINITION ORDER! */
        /***************************************/

        SHADER_MEMBER_TYPE_UINT,
        SHADER_MEMBER_TYPE_UVEC2,
        SHADER_MEMBER_TYPE_UVEC3,
        SHADER_MEMBER_TYPE_UVEC4,

        SHADER_MEMBER_TYPE_INT,
        SHADER_MEMBER_TYPE_IVEC2,
        SHADER_MEMBER_TYPE_IVEC3,
        SHADER_MEMBER_TYPE_IVEC4,

        SHADER_MEMBER_TYPE_FLOAT,
        SHADER_MEMBER_TYPE_VEC2,
        SHADER_MEMBER_TYPE_VEC3,
        SHADER_MEMBER_TYPE_VEC4,

        SHADER_MEMBER_TYPE_BOOL,
        SHADER_MEMBER_TYPE_BVEC2,
        SHADER_MEMBER_TYPE_BVEC3,
        SHADER_MEMBER_TYPE_BVEC4,

        SHADER_MEMBER_TYPE_MAT2x2,
        SHADER_MEMBER_TYPE_MAT2x3,
        SHADER_MEMBER_TYPE_MAT2x4,

        SHADER_MEMBER_TYPE_MAT3x2,
        SHADER_MEMBER_TYPE_MAT3x3,
        SHADER_MEMBER_TYPE_MAT3x4,

        SHADER_MEMBER_TYPE_MAT4x2,
        SHADER_MEMBER_TYPE_MAT4x3,
        SHADER_MEMBER_TYPE_MAT4x4,

        SHADER_MEMBER_TYPE_MAT2 = SHADER_MEMBER_TYPE_MAT2x2,
        SHADER_MEMBER_TYPE_MAT3 = SHADER_MEMBER_TYPE_MAT3x3,
        SHADER_MEMBER_TYPE_MAT4 = SHADER_MEMBER_TYPE_MAT4x4,

        /* Struct */
        SHADER_MEMBER_TYPE_AGGREGATE
    };

    /* Struct that describes a uniform/storage/push-constant block or its
     * members */
    struct UniformMember {
        /* Name of the member */
        char name[32];

        /* Array size (multidimensional arrays not supported) 
         * Can be zero if the size is defined at runtime. */
        uint32_t count;
        /* Length of the member/struct */
        uint32_t length;

        /* Offset inside of the struct */
        uint32_t offset;
        /* Padding relative to the binding */
        uint32_t absolutePadding;

        /* Type of the member */
        ShaderMemberType type;

        /* The members of the struct */
        std::vector<UniformMember> members;

        /* Hash of the uniform member */
        uint32_t hash;
    };

    struct UniformDescription {
        /* The name of the uniform */
        char name[32];

        /* type of the uniform */
        BindingType type;

        /* the set in the shader */
        uint32_t set;
        /* the binding in the shader */
        uint32_t binding;
        /* the size of the array */
        uint32_t count;
        /* stages where the uniform exists */
        StageFlags stages;

        /* if the type describes a uniform/storage/push-constant block */
        std::optional<UniformMember> block;
    };

    class ShaderReflection {
        /* This class exists for the purpose of future proofing in case a more
         * memory efficient reflection storage model is implemented */
    private:
        std::vector<UniformDescription> m_uniforms;

    public:
        constexpr ShaderReflection () = default;
        constexpr explicit ShaderReflection (
                std::vector<UniformDescription>&& desc) : m_uniforms (desc) {}
        constexpr ~ShaderReflection () = default;

        constexpr std::span<const UniformDescription> getShaderUniforms () const {
            return m_uniforms;
        }
    };

    /**
     * Wrapper to actual SPIR-V bytecode */
    struct ShaderBytecode {
    private:
        std::vector<uint32_t> m_data;

    public:
        ShaderBytecode () noexcept :
            m_data () { }

        ShaderBytecode (const ShaderBytecode& other) noexcept = default;
        ShaderBytecode (ShaderBytecode&& other) noexcept = default;

        ShaderBytecode& operator= (const ShaderBytecode& other) noexcept = default;
        ShaderBytecode& operator= (ShaderBytecode&& other) noexcept = default;

        ShaderBytecode (const uint32_t *data, size_t size) noexcept :
            m_data (data, data + size) { }

        [[nodiscard]]
        size_t size () const noexcept {
            return m_data.size () * 4;
        }

        [[nodiscard]]
        const uint32_t *data () const noexcept {
            return m_data.data ();
        }
    };

    struct ShaderInfo {
        bool reflect = true;
        std::span<const ShaderBytecode> bytecodes;
    };

    class IShader : public IObject {
    public:
        virtual ~IShader () = default;

        /* Get shader reflection data */
        virtual const ShaderReflection& getShaderReflection () const = 0;

        /**
         * Allocate a UniformSet that is compatible with this shader */
        virtual Ref<IUniformSet> allocateUniformSet (uint32_t set) = 0;
    };
};

#endif /* #define __SLRD_SHADER_HPP__ */
