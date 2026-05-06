/* SPDX-License-Identifer: LGPL-3.0-or-later */

#ifndef __SLRD_VULKAN_SHADER_HPP__
#define __SLRD_VULKAN_SHADER_HPP__

#include "vulkan/factory.hpp"
#include "vulkan/resource.hpp"
#include <slrd/shader.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include "pipelinemanager.hpp"


/* The api ignores the concept of DescriptorSets and DescriptorPools on the 
 * user level. But on the implementation level they are turned into a dynamic
 * device dependent state. */

namespace slrd {
    class VKDevice;
    class VKPipelineLayout;

    SLRD_RESOURCE_DEFINE_TYPE (VKShader);
    class VKShader : public IShader,
            public VKResource<VKShader> {
    private:
        std::shared_ptr<VKDevice> m_device = nullptr;
        std::vector<VkPipelineShaderStageCreateInfo> m_stages;
        
        ShaderReflection m_reflection;

        struct SpecializationInfo {
            /*SpecializationType type {};*/
            uint32_t id {};
            uint32_t size {};
            /* After deduplication what stages this specialization applies to */
            VkShaderStageFlags stages;
        };

        VkResult reflectShader (std::span<const ShaderBytecode> bytecode);

        PipelineLayoutInfoHash m_layoutHash {};

        /* stages used in this shader (in order of appearance) */
        std::vector<VkShaderStageFlagBits> m_stageFlags;
        /* specialization info */
        std::vector<SpecializationInfo> specInfo;

    public:
        VKShader () {}
        ~VKShader ();

        [[nodiscard]] auto& getDevice () const {
            return m_device;
        }

        [[nodiscard]] const auto& getStages () const {
            return m_stages;
        }

        int init (std::shared_ptr<VKDevice> device, const ShaderInfo& config);

        /* Get pipeline layout for the shader */
        VKPipelineLayout *getOrCreatePipelineLayout ();
        VKPipelineLayout *grabOrCreatePipelineLayout ();

        std::shared_ptr<IUniformSet> allocateUniformSet (uint32_t set) final override;

        const ShaderReflection& getShaderReflection () const final override;
    };

    inline std::shared_ptr<VKShader> createVKShader (std::shared_ptr<VKDevice> device,
            const ShaderInfo& info) {
        return makeResource<VKShader> (device, info);
    }
};

#endif /* #define __SLRD_VULKAN_SHADER_HPP__ */
