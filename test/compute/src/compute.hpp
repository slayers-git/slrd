/* This file is an example taken from vulkan-tutorial.com on compute shaders
 * it implements the particle system that is calculated on the gpu */

#ifndef __COMPUTE_HPP__
#define __COMPUTE_HPP__

#include <slrdframework/app.hpp>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static const std::array<slrd::VertexBindingDescription, 1>&
    getVertexBindingDescription () {
        static const std::array<slrd::VertexBindingDescription, 1> binding = {
            slrd::VertexBindingDescription {
                .binding = 0,
                .stride = sizeof (Vertex),
                .inputRate = slrd::VERTEX_INPUT_RATE_VERTEX,
            }
        };

        return binding;
    }

    static const std::array<slrd::VertexAttributeDescription, 2>&
    getVertexAttributeDescription () {
        static const std::array<slrd::VertexAttributeDescription, 2> attr = {
            slrd::VertexAttributeDescription {
                .location = 0,
                .binding = 0,
                .offset = 0,
                .format = slrd::FORMAT_RGB32_SFLOAT
            },

            slrd::VertexAttributeDescription {
                .location = 1,
                .binding = 0,
                .offset = offsetof (Vertex, color),
                .format = slrd::FORMAT_RGBA32_SFLOAT,
            }
        };

        return attr;
    }
};

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;
};

struct UBO {
    glm::mat4 projection;
    float delta alignas (16);
};

class Compute : public App {
private:
    slrd::Ref<slrd::IPipeline> m_computePipeline;
    slrd::Ref<slrd::IPipeline> m_graphicsPipeline;

    slrd::Ref<slrd::IRenderPass> m_renderPass;

    slrd::Ref<slrd::ICommandBuffer> m_commandBuffer;

    uint32_t m_currentSSBO = 0;
    slrd::Ref<slrd::IBuffer> m_particleSSBOs[2];

    slrd::Ref<slrd::IBuffer> m_ubo;

    slrd::Ref<slrd::IUniformSet> m_graphicsUniformSet;
    slrd::Ref<slrd::IUniformSet> m_computeUniformSet;

    UBO *m_uboMap;

    slrd::Ref<slrd::ITexture> m_texture;
    slrd::Ref<slrd::ITextureView> m_textureView;

    slrd::Ref<slrd::ISampler> m_sampler;

    void initParticleBuffer ();
    void draw () override;

    void updateCompute ();

    const uint32_t MAX_PARTICLES = 1024;

public:
    Compute ();
    ~Compute ();
};

#endif /* #define __COMPUTE_HPP__ */
