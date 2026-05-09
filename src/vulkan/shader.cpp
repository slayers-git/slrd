/* SPDX-License-Identifer: LGPL-3.0-or-later */

#include "shader.hpp"
#include "vulkan/error.hpp"
#include <map>

#include "device.hpp"
#include "vulkan/pipelinelayout.hpp"
#include "vulkan/uniformset.hpp"

#include <spirv_reflect.h>
#include <xxhash.h>

namespace slrd {
    static inline auto getMemberHash (UniformMember& member) {
        XXH64_state_t *state = XXH64_createState ();

        SLRD_DEBUG_CRIT_IF (!state,
                "Failed to create a state for the hashing function");
        SLRD_DEBUG_CRIT_IF (XXH64_reset (state, 0),
                "Failed to create a state for the hashing function");

        XXH64_update (state, &member, offsetof (UniformMember, members) +
                sizeof (UniformMember::members));

        const auto hash = XXH64_digest (state);
        XXH64_freeState (state);
        
        return hash;
    }

    static std::optional<UniformMember> readUniformMember (const SpvReflectBlockVariable& block) {
        UniformMember member{};

        RETURN_LOG_ERROR_IF (block.array.dims_count > 1, std::nullopt,
                "Multidimensional arrays are not supported!");

        const auto array_count = block.array.dims[0];
        const auto& spv_type = *block.type_description;
        ShaderMemberType type = SHADER_MEMBER_TYPE_AGGREGATE;

        member.count = array_count || (spv_type.type_flags &
                SPV_REFLECT_TYPE_FLAG_ARRAY) ? array_count : 1;
        member.length = block.size;
        member.offset = block.offset;
        member.absolutePadding = block.absolute_offset;

        if (!block.member_count) {
            if (spv_type.type_flags & SPV_REFLECT_TYPE_FLAG_INT) {
                type = SHADER_MEMBER_TYPE_INT;
                if (!spv_type.traits.numeric.scalar.signedness) {
                    type = SHADER_MEMBER_TYPE_UINT;
                }
            } else if (spv_type.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) {
                type = SHADER_MEMBER_TYPE_FLOAT;
            } else if (spv_type.type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) {
                type = SHADER_MEMBER_TYPE_BOOL;
            }

            if (spv_type.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR &&
                    !(spv_type.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX)) {
                const auto nr_comp = spv_type.traits.numeric.vector.component_count;

                RETURN_LOG_ERROR_IF (!nr_comp || nr_comp > 4, std::nullopt,
                        "Invalid number of components in vector {}!", member.name);

                type = static_cast<ShaderMemberType> (type + nr_comp - 1);
            } else if (spv_type.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) {
                const auto cols = spv_type.traits.numeric.matrix.column_count;
                const auto rows = spv_type.traits.numeric.matrix.row_count;

                RETURN_LOG_ERROR_IF (cols < 2 || cols > 4 || rows < 2 || rows > 4, std::nullopt,
                        "Invalid dimensions for matrix {}!", member.name);

                type = static_cast<ShaderMemberType> (
                            SHADER_MEMBER_TYPE_MAT2 + (3 * (cols - 2)) + rows - 2);
            }
        }

        member.type = type;

        std::memcpy (member.name, block.name, 32);

        uint64_t combined_hash = 0;
        member.members.reserve (block.member_count);
        for (unsigned i = 0; i < block.member_count; ++i) {
            auto child = readUniformMember (block.members[i]);
            if (!child.has_value ())
                return std::nullopt;

            /* Combine the hashes */
            combined_hash ^= child->hash * 0x9ddfea08eb382d69;
            member.members.emplace_back (std::move (child.value ()));
        }

        const auto hash = getMemberHash (member);
        member.hash = combined_hash ^ hash * 0x9ddfea08eb382d69;
        return member;
    }

    static std::optional<UniformMember> readUniformBlock (const SpvReflectDescriptorBinding *binding) {
        auto member = readUniformMember (binding->block);
        if (!member.has_value ())
            return std::nullopt;

        return member;
    }

    /* FIXME: Maybe store the duplicates in a separate heap to cut down on
     * memory usage at the cost of not being able to use the date when the
     * shader is destroyed (small price to pay, I think) */
    VkResult VKShader::reflectShader (std::span<const ShaderBytecode> bytecode) {
        std::vector<DescriptorSet> descriptor_sets;
        VkPushConstantRange push_constants {};
        std::vector<SpecializationInfo> spec_infos;
        std::map<uint32_t, uint32_t> used_sets;
        std::vector<VkShaderStageFlagBits> used_stages (bytecode.size ());

        std::vector<std::vector<UniformDescription>> uniform_sets;

        for (uint32_t i = 0; i < bytecode.size (); ++i) {
            spv_reflect::ShaderModule module (bytecode[i].size (), bytecode[i].data ());

            SpvReflectResult result = module.GetResult ();
            RETURN_LOG_ERROR_IF (result != SPV_REFLECT_RESULT_SUCCESS, VK_ERROR_UNKNOWN,
                    "Failed to create shadermodule for reflection");

            VkShaderStageFlags current_stage = module.GetShaderStage ();
            used_stages[i] = (VkShaderStageFlagBits)current_stage;

            uint32_t nr_bindings = 0;
            result = module.EnumerateDescriptorBindings (&nr_bindings, nullptr);
            WRAP_COND_RETURN (result != SPV_REFLECT_RESULT_SUCCESS, VK_ERROR_UNKNOWN);

            std::vector<SpvReflectDescriptorBinding *> bindings (nr_bindings);
            result = module.EnumerateDescriptorBindings (&nr_bindings, bindings.data ());
            WRAP_COND_RETURN (result != SPV_REFLECT_RESULT_SUCCESS, VK_ERROR_UNKNOWN);

            /* Reflect and save reflection infos */
            for (uint32_t i = 0; i < nr_bindings; ++i) {
                UniformDescription uniform;
                auto binding = bindings[i];

                uint32_t set = binding->set;
                RETURN_LOG_ERROR_IF (set > VKPipelineLayout::MAX_SETS, VK_ERROR_UNKNOWN,
                        "Set number is greater than max allowed set count!");

                /* Length is 0 for every non-uniform/storage block */
                uniform.set     = set;
                uniform.binding = binding->binding;
                uniform.count   = binding->count;
                uniform.stages  = getSLRDStageFlags (current_stage);

                std::memcpy (uniform.name, binding->name, 32);

                switch (binding->descriptor_type) {
                    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
                        uniform.type = slrd::BINDING_TYPE_SAMPLER;
                        break;
                    case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                        uniform.type = slrd::BINDING_TYPE_COMBINED_TEXTURE_SAMPLER;
                        break;
                    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER: {
                        uniform.type = slrd::BINDING_TYPE_UNIFORM_BUFFER;
                        auto block = readUniformBlock (binding);
                        if (!block.has_value ())
                            return VK_ERROR_UNKNOWN;

                        uniform.block = std::move (block);
                        break;
                    }
                    case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: {
                        uniform.type = slrd::BINDING_TYPE_UNIFORM_BUFFER_DYNAMIC;
                        auto block = readUniformBlock (binding);
                        if (!block.has_value ())
                            return VK_ERROR_UNKNOWN;

                        uniform.block = std::move (block);
                        break;
                    }
                    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER: {
                        uniform.type = slrd::BINDING_TYPE_STORAGE_BUFFER;
                        auto block = readUniformBlock (binding);
                        if (!block.has_value ())
                            return VK_ERROR_UNKNOWN;

                        uniform.block = std::move (block);
                        break;
                    }
                    case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: {
                        uniform.type = slrd::BINDING_TYPE_STORAGE_BUFFER_DYNAMIC;
                        auto block = readUniformBlock (binding);
                        if (!block.has_value ())
                            return VK_ERROR_UNKNOWN;

                        uniform.block = std::move (block);
                        break;
                    }
                    case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                        uniform.type = slrd::BINDING_TYPE_TEXTURE;
                        break;
                    default:
                        RETURN_LOG_ERROR_IF (true, VK_ERROR_UNKNOWN, "Unsupported descriptor type!");
                        break;
                }

                /* dedup and insert */
                auto it = used_sets.find (set);
                bool found = false;
                if (it != used_sets.end ()) {
                    auto& uniform_set = uniform_sets[it->second];
                    for (uint32_t j = 0; j < uniform_set.size (); ++j) {
                        if (uniform.binding == uniform_set[j].binding) {
                            RETURN_LOG_ERROR_IF (std::strncmp (uniform.name, uniform_set[j].name, 32) != 0, 
                                    VK_ERROR_UNKNOWN,
                                    "Shader stages use the same uniform location, different uniform names");
                            RETURN_LOG_ERROR_IF (uniform.type != uniform_set[j].type, VK_ERROR_UNKNOWN,
                                    "Shader stages use the same uniform location, but different types");
                            RETURN_LOG_ERROR_IF (uniform.count != uniform_set[j].count, VK_ERROR_UNKNOWN,
                                    "Shader stages use the same uniform location, but have different array lengths");
                            RETURN_LOG_ERROR_IF (uniform.block.has_value () && 
                                    !(uniform.block->hash != uniform_set[j].block->hash), VK_ERROR_UNKNOWN,
                                    "Shader stages use the same uniform location, but have different types");

                            found = true;
                            uniform_set[j].stages |= getSLRDStageFlags (current_stage);
                            break;
                        }
                    }

                    if (!found) {
                        uniform_set.emplace_back (std::move (uniform));
                    }
                } else {
                    uniform_sets.push_back ({ std::move (uniform) });
                    used_sets.emplace (set, uniform_sets.size () - 1);
                }
            }

            uint32_t nr_push_block = 0;
            result = module.EnumeratePushConstantBlocks (&nr_push_block, nullptr);
            WRAP_COND_RETURN (result != SPV_REFLECT_RESULT_SUCCESS, {});

            if (nr_push_block) {
                std::vector<SpvReflectBlockVariable *> pcBlocks (nr_push_block);
                result = module.EnumeratePushConstantBlocks (&nr_push_block, pcBlocks.data ());
                WRAP_COND_RETURN (result != SPV_REFLECT_RESULT_SUCCESS, {});
                
                push_constants.size = pcBlocks[0]->size;
                push_constants.stageFlags |= current_stage;
                push_constants.offset = 0;
            }

            uint32_t nr_spec = 0;
            result = module.EnumerateSpecializationConstants (&nr_spec, nullptr);
            WRAP_COND_RETURN (result != SPV_REFLECT_RESULT_SUCCESS, {});

            std::vector<SpvReflectSpecializationConstant *> specs (nr_spec);

            result = module.EnumerateSpecializationConstants (&nr_spec, specs.data ());
            WRAP_COND_RETURN (result != SPV_REFLECT_RESULT_SUCCESS, {});

            for (uint32_t i = 0; i < nr_spec; ++i) {
                const auto& sc = specs[i];
                SpecializationInfo si;
                si.id = sc->constant_id;
                si.stages = current_stage;
                
                bool found = false;
                for (uint32_t j = 0; j < spec_infos.size (); ++j) {
                    if (si.id == spec_infos[i].id) {
                        si.stages |= current_stage;
                        found = true;
                        break;
                    }
                }

                if (!found)
                    spec_infos.push_back (si);
            }
        }

        /* Convert reflection data into descriptor set infos */
        descriptor_sets.resize (used_sets.size ());
        for (const auto&[set_id, array_id] : used_sets) {
            descriptor_sets[set_id].set = set_id;
            descriptor_sets[set_id].bindings.reserve (uniform_sets[array_id].size ());

            for (const auto& uniform : uniform_sets[array_id]) {
                VkDescriptorSetLayoutBinding binding {};
                binding.binding = uniform.binding;
                binding.descriptorCount = uniform.count;
                binding.descriptorType = getVkDescriptorType (uniform.type);
                binding.stageFlags = getVkShaderStageFlags (uniform.stages);

                descriptor_sets[set_id].bindings.emplace_back (std::move (binding));
            } 
        }

        std::vector<UniformDescription> uniform_descriptions;
        for (const auto& set : uniform_sets) {
            uniform_descriptions.insert (uniform_descriptions.end (),
                    set.cbegin (), set.cend ());
        }

        m_reflection = ShaderReflection (std::move (uniform_descriptions));
        m_stageFlags = std::move (used_stages);

        PipelineLayoutInfo info;

        std::sort (descriptor_sets.begin (), descriptor_sets.end ());
        info.sets = std::move (descriptor_sets);
        info.pushConstants = std::move (push_constants);
        info.calculateHash ();

        m_layoutHash = info.hash;
        m_device->getPipelineManager ()->addPipelineLayoutInfo (info);

        return VK_SUCCESS;
    }

    VkResult createShaderStage (
            VkDevice device,
            VkShaderStageFlagBits stage,
            const uint32_t *bytecode, uint32_t size,
            VkPipelineShaderStageCreateInfo *info) {
        VkShaderModule module;
        VkShaderModuleCreateInfo moduleInfo {};
        moduleInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        moduleInfo.pCode = bytecode;
        moduleInfo.codeSize = size;

        VK_WRAP_RETURN_RESULT_LOGERROR (vkCreateShaderModule (device, &moduleInfo,
                    nullptr, &module),
                "Failed to create the shader stage");

        VkPipelineShaderStageCreateInfo stageInfo {};
        stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stageInfo.stage = stage;
        stageInfo.module = module;
        stageInfo.pName = "main";

        *info = stageInfo;
        return VK_SUCCESS;
    }


    int VKShader::init (VKDevice *device, const ShaderInfo& config) {
        setParentDevice (device);

        WRAP_COND_RETURN (reflectShader (config.bytecodes) != VK_SUCCESS, -1);

        std::vector<VkPipelineShaderStageCreateInfo> stages (config.bytecodes.size ());
        for (uint32_t i = 0; i < stages.size (); ++i) {
            VkResult result = createShaderStage (device->getVkDevice (), m_stageFlags[i],
                    (uint32_t *)config.bytecodes[i].data (),
                    config.bytecodes[i].size (), &stages[i]);

            WRAP_COND_RETURN (result != VK_SUCCESS, -1);
        }

        m_stages = stages;

        device->allocate (OBJECT_TYPE_SHADER, 0);
        return 0;
    }

    VKPipelineLayout *VKShader::getOrCreatePipelineLayout () {
        return m_device->getPipelineManager ()->getOrCreatePipelineLayout (m_layoutHash);
    }

    VKPipelineLayout *VKShader::grabOrCreatePipelineLayout () {
        return m_device->getPipelineManager ()->grabOrCreatePipelineLayout (m_layoutHash);
    }

    Ref<IUniformSet> VKShader::allocateUniformSet (uint32_t set) {
        return Ref<IUniformSet>::adopt (
            getOrCreatePipelineLayout ()->allocateUniformSet (set)
        );
    }

    const ShaderReflection& VKShader::getShaderReflection () const {
        return m_reflection;
    }

    VKShader::~VKShader () {
        for (auto& stage : m_stages) {
            if (stage.module)
                vkDestroyShaderModule (m_device->getVkDevice (), stage.module, nullptr);
        }

        if (m_layoutHash) {
            m_device->getPipelineManager ()->releasePipelineLayout (m_layoutHash);
        }
    }
}
