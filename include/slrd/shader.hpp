/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_SHADER_HPP__
#define __SLRD_SHADER_HPP__

#include "slrd/format.hpp"
#include "slrd/pipeline.hpp"
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

        constexpr ProxyArray<UniformDescription> getShaderUniforms () const {
            return m_uniforms;
        }
    };

    struct ShaderBytecode {
        std::vector<uint8_t> bytecode;

        ShaderBytecode () : bytecode () {}

        ShaderBytecode (ShaderBytecode&& other) = default;
        ShaderBytecode& operator= (ShaderBytecode&& other) = default;

        template<size_t Size>
        ShaderBytecode (uint32_t (&code)[Size]) :
            bytecode ((uint8_t *)code, (uint8_t *)code + Size * 4) {

        }

        ShaderBytecode (uint8_t *code, uint32_t size) :
            bytecode (code, code + size) { }

        ShaderBytecode (uint32_t *code, uint32_t size) :
            bytecode ((uint8_t *)code, (uint8_t *)code + size * 4) { }

        template<typename T>
        ShaderBytecode (const slrd::ProxyArray<T>& data) :
            bytecode ((uint8_t *)data.begin (), (uint8_t *)data.end()) { }

        uint32_t size () const {
            return bytecode.size ();
        }

        const uint8_t *data () const {
            return bytecode.data ();
        }
    };

    struct ShaderInfo {
        bool reflect = true;
        slrd::ProxyArray<ShaderBytecode> bytecodes;
    };

    class IShader {
    protected:
        ShaderType m_shaderType;

    public:
        virtual ~IShader () = default;

        ShaderType getType () const {
            return m_shaderType;
        }

        /* Get shader reflection data */
        virtual const ShaderReflection& getShaderReflection () const = 0;

        virtual std::shared_ptr<IUniformSet> allocateUniformSet (uint32_t set) = 0;
    };
};

#endif /* #define __SLRD_SHADER_HPP__ */
