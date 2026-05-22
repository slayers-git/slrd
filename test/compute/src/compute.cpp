#include "compute.hpp"
#include <slrd/slrd.hpp>

#include <slrd/uniformupdater.hpp>

#include <random>

#include <slrdframework/util.hpp>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_transform.hpp>

void Compute::initParticleBuffer () {
    std::default_random_engine rndEngine((unsigned)time(nullptr));
    std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

    // Initial particle positions on a circle
    std::vector<Particle> particles (MAX_PARTICLES);
    for (auto& particle : particles) {
        float r = 0.25f * sqrt (rndDist (rndEngine));
        float theta = rndDist (rndEngine) * 2 * M_PI;
        float x = r * cos (theta) * 800 / 600;
        float y = r * sin (theta);

        particle.position = glm::vec2 (x, y);
        particle.velocity = glm::normalize (glm::vec2(x,y)) * 0.00025f;
        particle.color = glm::vec4 (rndDist (rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
    }

    slrd::CommandPoolInfo pool_info;
    pool_info.queue = m_commandQueue.get ();
    pool_info.flags = slrd::COMMAND_POOL_FLAG_TRANSIENT;
    auto pool = m_device->createCommandPool (pool_info);
    if (!pool)
        throw std::runtime_error ("Failed to create a command pool");
    auto cmd_buffer = pool->allocate ({});
    if (!cmd_buffer)
        throw std::runtime_error ("Failed to create a command buffer");

    m_particleSSBOs[0] = util::createBufferWithData (m_device.get (),
            cmd_buffer.get (), m_commandQueue.get (),
            slrd::BUFFER_USAGE_VERTEX_BUFFER | slrd::BUFFER_USAGE_STORAGE_BUFFER,
            particles.data (), sizeof (Particle) * MAX_PARTICLES);

    pool->reset ();

    m_particleSSBOs[1] = util::createBufferWithData (m_device.get (),
            cmd_buffer.get (), m_commandQueue.get (),
            slrd::BUFFER_USAGE_VERTEX_BUFFER | slrd::BUFFER_USAGE_STORAGE_BUFFER,
            particles.data (), sizeof (Particle) * MAX_PARTICLES);
}

Compute::Compute () :
        App ({ "Compute", slrd::API_VULKAN, 800, 600, false }) {
    slrd::RenderPassInfo rpInfo;
    slrd::RenderPassAttachment color, depth;
    color.format = m_swapchain->getFormat ();
    color.loadOp = slrd::LOAD_OPERATION_CLEAR;
    color.storeOp = slrd::STORE_OPERATION_STORE;
    color.initialLayout = slrd::TEXTURE_LAYOUT_UNDEFINED;
    color.finalLayout   = slrd::TEXTURE_LAYOUT_SWAPCHAIN_SRC;
    color.presentable = true;

    depth.format = slrd::FORMAT_D24UNORMS8UINT;
    depth.loadOp = slrd::LOAD_OPERATION_CLEAR;
    depth.storeOp = slrd::STORE_OPERATION_DONT_CARE;
    depth.initialLayout = slrd::TEXTURE_LAYOUT_UNDEFINED;
    depth.finalLayout   = slrd::TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT;
    depth.presentable = true;

    rpInfo.colorAttachments = { &color, 1 };
    rpInfo.depthAttachment = depth;
    m_renderPass = m_device->createRenderPass (rpInfo);
    if (!m_renderPass) {
        throw std::runtime_error ("Failed to create the renderpass");
    }

    m_renderPass->setDepthView (m_depthView.get ());

    {
        slrd::Ref<slrd::IShader> gShader = util::loadShader (m_device.get (),
                { "shaders/simple.vert.spv", "shaders/simple.frag.spv" });

        slrd::GraphicsPipelineInfo pInfo;
        pInfo.shader = gShader.get ();
        pInfo.rasterizerConfig.cullMode = slrd::CULL_MODE_NONE;
        pInfo.depthStencilConfig.depthFormat = slrd::FORMAT_D24UNORMS8UINT;
        pInfo.depthStencilConfig.depthTestEnabled = true;
        pInfo.depthStencilConfig.depthWriteEnabled = true;
        pInfo.depthStencilConfig.compareOperator = slrd::COMPARE_OPERATOR_LESS;
        slrd::ColorBlendAttachment cba;
        cba.blendEnabled = false;
        cba.colorWriteMask = slrd::COLOR_MASK_RGBA;
        pInfo.colorBlendConfig.attachments = { &cba, 1 };
        pInfo.vertexConfig.vertexBindings = Vertex::getVertexBindingDescription ();
        pInfo.vertexConfig.attributeDescs = Vertex::getVertexAttributeDescription ();
        pInfo.inputAssembly.topology = slrd::PRIMITIVE_TOPOLOGY_POINT_LIST;

        m_graphicsPipeline = m_device->createGraphicsPipeline (pInfo);
        if (!m_graphicsPipeline) {
            throw std::runtime_error ("Failed to create the pipeline");
        }
    }


    {
        slrd::Ref<slrd::IShader> cShader = util::loadShader (m_device.get (),
                { "shaders/particle.comp.spv" });

        slrd::ComputePipelineInfo pInfo;
        pInfo.shader = cShader.get ();

        m_computePipeline = m_device->createComputePipeline (pInfo);
        if (!m_computePipeline) {
            throw std::runtime_error ("Failed to create the pipeline");
        }
    }

    m_commandBuffer = m_commandPool->allocate ({});
    if (!m_commandBuffer)
        throw std::runtime_error ("Failed to get the command buffer");

    {
        slrd::BufferInfo bufInfo;
        bufInfo.usage = slrd::BUFFER_USAGE_UNIFORM_BUFFER;
        bufInfo.gpu = true;
        bufInfo.size = sizeof (UBO);
        bufInfo.coherent = true;
        bufInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;
        m_ubo = m_device->createBuffer (bufInfo);
        if (!m_ubo)
            throw std::runtime_error ("Failed to create the UBO");

        m_uboMap = static_cast<UBO *> (m_ubo->map ());
        if (!m_ubo)
            throw std::runtime_error ("Failed to create the UBO mapping!");
    }
    initParticleBuffer ();

    m_graphicsUniformSet = m_graphicsPipeline->allocateUniformSet (0);
    if (!m_graphicsPipeline)
        throw std::runtime_error ("Failed to get the graphics uniform set");

    m_computeUniformSet  = m_computePipeline->allocateUniformSet (0);
    if (!m_computeUniformSet)
        throw std::runtime_error ("Failed to get the compute uniform set");

    {
        slrd::UniformUpdater updater;
        updater
            .updateUniformBuffer (0, m_ubo.get (), sizeof (UBO), 0);

        m_graphicsUniformSet->updateUniforms (updater.get ());
        m_computeUniformSet->updateUniforms (updater.get ());
    }

    initImGUI (m_renderPass.get ());
}

void Compute::updateCompute () {
    uint32_t currentSSBO = (m_currentSSBO + 1) % 2;
    slrd::UniformUpdater updater;
    updater
        .updateStorageBuffer (1, m_particleSSBOs[m_currentSSBO].get (),
                sizeof (Particle) * MAX_PARTICLES)
        .updateStorageBuffer (2, m_particleSSBOs[currentSSBO].get (),
                sizeof (Particle) * MAX_PARTICLES);

    m_currentSSBO = currentSSBO;
    m_computeUniformSet->updateUniforms (updater.get ());
}

void Compute::draw () {
    m_fence->wait ();
    m_commandPool->reset ();

    m_profiler->newFrame ();

    auto niTexView = nextFrame ();
    if (wasSwapchainRecreated ()) {
        /* Recalculate matrix for view and for projection */
        m_uboMap->projection = glm::perspective<float> (90.f,
                (float)getWidth () / getHeight (),
                0.1, 100.f);
        m_renderPass->setDepthView (m_depthView.get ());
    }

    m_renderPass->setTextureView (0, niTexView);
    m_uboMap->delta = getDeltaTime ();

    const auto w = getWidth (),
               h = getHeight ();
    slrd::Scissor sc = { 0, 0, w, h };
    slrd::Viewport vp = { 0, 0, (float)w, (float)h };

    updateCompute ();

    m_commandBuffer->begin ();

        m_profiler->startScope ("ComputeShader");
        m_commandBuffer->bindComputePipeline (m_computePipeline.get ());
        {
            slrd::IUniformSet *sets[] = {
                m_computeUniformSet.get ()
            };
            m_commandBuffer->bindSets (sets);
        }
        m_commandBuffer->dispatch ({ MAX_PARTICLES / 256, 1, 1 });
        m_profiler->endScope ("ComputeShader");

        slrd::BufferBarrierInfo bar;
        bar.buffer = m_particleSSBOs[m_currentSSBO].get ();
        bar.size   = sizeof (Particle) * MAX_PARTICLES;
        bar.srcAccessFlags = slrd::MEMORY_ACCESS_FLAG_WRITE;
        bar.dstAccessFlags = slrd::MEMORY_ACCESS_FLAG_READ;

        m_commandBuffer->pipelineBufferBarrier (bar);
    
        m_profiler->startScope ("GraphicsPass");
        slrd::RenderPassColorClearValue cv = { 0, 0, 0, 0 };
        slrd::RenderPassBeginInfo begInfo;
        begInfo.colorClearValues = { &cv, 1 };
        m_commandBuffer->beginRenderPass (m_renderPass.get (), begInfo); {
            m_commandBuffer->bindGraphicsPipeline (m_graphicsPipeline.get ());
            {
                slrd::IUniformSet *sets[] = {
                    m_graphicsUniformSet.get ()
                };
                m_commandBuffer->bindSets (sets);
            }

            m_commandBuffer->setViewport (vp);
            m_commandBuffer->setScissor (sc);

            m_commandBuffer->bindVertexBuffer (m_particleSSBOs[m_currentSSBO].get (), 0);
            m_commandBuffer->draw (MAX_PARTICLES, 1);
        } m_commandBuffer->endRenderPass ();
        m_profiler->endScope ("GraphicsPass");

        /*ImGui::Begin ("Profiler");*/
        /**/
        /*auto data = m_profiler->getData ();*/
        /*for (const auto scope : data) {*/
        /*    ImGui::TextUnformatted (*/
        /*        std::format ("- {}: {}ms", scope.name, scope.duration).c_str ());*/
        /*}*/
        /**/
        /*ImGui::End ();*/

    m_commandBuffer->end ();

    slrd::ICommandBuffer *cmd_buffers[] = {
        m_commandBuffer.get ()
    };
    slrd::SubmitInfo submitInfo;
    submitInfo.fence = getCurrentFence ();
    submitInfo.commandBuffers = { cmd_buffers, 1 };

    m_profiler->startScope ("Submit");
    m_commandQueue->submit (submitInfo);
    m_profiler->endScope ("Submit");

    m_profiler->startScope ("Present");
    present ();
    m_profiler->endScope ("Present");

    m_profiler->endFrame ();

    //m_profiler->printData ();
}

Compute::~Compute () {
    m_commandQueue->wait ();
}
