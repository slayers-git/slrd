#include "slrd/error.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <array>

#include <slrd/api.hpp>
#include <slrd/slrd.hpp>
#include <slrd/device.hpp>

#include <slrd/platform/vulkan.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vector>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtc/type_ptr.hpp>

static void draw ();
static void createSwapchain ();
static void createPipelineAndRenderpass ();

/*struct UniformBuffer {*/
/*    glm::mat4 proj;*/
/*    glm::mat4 view;*/
/*    glm::mat4 model;*/
/*};*/

struct Vertex {
    glm::vec3 position;
    glm::vec3 color;

    static const std::array<slrd::VertexBindingDescription, 1>& getBindingDescriptions () {
        static std::array<slrd::VertexBindingDescription, 1> bindingInfo = {
            slrd::VertexBindingDescription {
                .binding = 0,
                .stride = sizeof (Vertex),
                .inputRate = slrd::VERTEX_INPUT_RATE_VERTEX
            }
        };

        return bindingInfo;
    }

    static const std::array<slrd::VertexAttributeDescription, 2>& getAttributeDescription () {
        static std::array<slrd::VertexAttributeDescription, 2> attrInfo = {
            slrd::VertexAttributeDescription {
                .location = 0,
                .binding =  0,
                .offset  = offsetof (Vertex, position),
                .format  = slrd::FORMAT_RGB32_SFLOAT,
            },
            slrd::VertexAttributeDescription {
                .location = 1,
                .binding  = 0,
                .offset   = offsetof (Vertex, color),
                .format   = slrd::FORMAT_RGB32_SFLOAT,
            }
        };

        return attrInfo;
    }
};

static const Vertex s_vertices[] = {
    { {  0.0f, -0.5f, 0 }, { 1, 0, 0 } },
    { { -0.5f,  0.5f, 0 }, { 0, 1, 0 } },
    { {  0.5f,  0.5f, 0 }, { 0, 0, 1 } }
};

struct App {
    slrd::DevicePtr m_device;
    slrd::SwapchainPtr m_swapchain;

    slrd::RenderPassPtr m_renderPass;
    slrd::PipelinePtr m_pipeline;

    /* This program uses one inflight frame */

    slrd::FencePtr m_fence;

    slrd::BufferPtr m_uniformBuffer;
    slrd::BufferPtr m_triangleBuffer;

    slrd::CommandQueuePtr m_commandQueue;
    slrd::ICommandBuffer *m_commandBuffer;

    bool m_shouldRecreateSwapchain = false;

    SDL_Window *m_window;
    SDL_Event  m_event;

    App () {
        if (SDL_Init (SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
            std::cerr << "Failed to initilize SDL.\n";
            exit (1);
        }

        SDL_Vulkan_LoadLibrary (nullptr);

        m_window = SDL_CreateWindow ("Vulkan Test", 0, 0, 800, 600, SDL_WINDOW_VULKAN);
        if (!m_window) {
            std::cerr << "Failed to initialize the window: " << SDL_GetError () << '\n';
            exit (1);
        }


        std::vector<const char *> instanceExtensions;
        {
            uint32_t extCount = 0;
            if (!SDL_Vulkan_GetInstanceExtensions (m_window, &extCount, nullptr)) {
                std::cerr << "Failed to get instance extensions: " << SDL_GetError () << '\n';
                exit (1);
            }
            
            instanceExtensions.resize (extCount);
            if (!SDL_Vulkan_GetInstanceExtensions (m_window, &extCount, instanceExtensions.data ())) {
                std::cerr << "Failed to get instance extensions: " << SDL_GetError () << '\n';
                exit (1);
            }
        }

        slrd::APIConfig config;
        config.app_name = "Test SLRD";
        config.dev_name = "slayer";
        config.engine_name = "slrd";
        config.app_version = { 1, 0, 0 }; 
        config.engine_version = { 0, 0, 1 };
        config.instance_extensions = instanceExtensions;
        config.debug = true;

        auto apis = slrd::querySupportedAPIs ();
        if (!(apis & slrd::API_VULKAN)) {
            std::cerr << "Implementation doesn't support Vulkan\n";
            exit (1);
        }

        if (slrd::init (slrd::API_VULKAN, config)) {
            std::cerr << slrd::getErrorString ();
            exit (1);
        }

        {
            static const char *devext[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

            slrd::DeviceConfig devconf;
            devconf.device_extensions = devext;
            m_device = slrd::createDevice (devconf);
            if (!m_device) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }
        }

        /* Create the window surface */
        {
            auto *data = slrd::platform::vulkan::getVulkanAPIData ();

            VkSurfaceKHR vksurface;
            if (!SDL_Vulkan_CreateSurface (m_window, data->instance, &vksurface)) {
                std::cerr << "Failed to create surface: " << SDL_GetError () << '\n';
                exit (1);
            }

            slrd::SurfaceInfo info;
            info.apiData.ptr = vksurface;
            auto surface = slrd::createSurface (info);
            if (!surface) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }

            /* Create the swapchain with the surface */
            slrd::SwapchainInfo swpInfo;
            swpInfo.surface = surface.get ();
            swpInfo.width = 800;
            swpInfo.height = 600;
            swpInfo.requestedImages = 2;

            m_swapchain = m_device->createSwapchain (swpInfo);
            if (!m_swapchain) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }

            m_fence = m_device->createFence (true);
            if (!m_fence) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }
        }

        {
            /* Since we use the Vulkan API here, it's important to understand that
             * terminology of CommandQueue comes from D3D12. In reality, what is
             * happening is that we create a VkCommandPool here.
             *
             * As such, commiting actually happens to one queue, requested 
             * during the creation of IDevice. */
            slrd::CommandQueueInfo cInfo;
            cInfo.flags = slrd::COMMAND_QUEUE_GRAPHICS;
            m_commandQueue = m_device->createCommandQueue (cInfo);
            if (!m_commandQueue) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }

            m_commandBuffer = m_commandQueue->getCommandBuffer ();
            if (!m_commandBuffer) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }
        }

        init ();
    }

    void init () {
        /* Initialize the triangle */
        {
            slrd::BufferInfo bi;
            bi.usage = slrd::BUFFER_USAGE_VERTEX_BUFFER;
            bi.coherent = true;
            bi.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;
            bi.size = sizeof (s_vertices);

            m_triangleBuffer = m_device->createBuffer (bi);
            if (!m_triangleBuffer) {
                std::cerr << "Failed to create buffer: " << slrd::getErrorString ();
                exit (1);
            }
            
            /* A very cheap way to do this. */
            if (m_triangleBuffer->setBuffer (s_vertices, sizeof (s_vertices))) {
                std::cerr << "Failed to set triangle buffer data: " << slrd::getErrorString ();
                exit (1);
            }
        }

        /* Initialize the uniform buffer for the triangle */
        /*{*/
        /*    slrd::BufferInfo bi;*/
        /*    bi.usage = slrd::BUFFER_USAGE_UNIFORM_BUFFER;*/
        /*    bi.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;*/
        /*    bi.coherent = true;*/
        /*    bi.size = sizeof (UniformBuffer);*/
        /**/
        /*    m_uniformBuffer = m_device->createBuffer (bi);*/
        /*    if (!m_uniformBuffer) {*/
        /*        std::cerr << "Failed to create buffer: " << slrd::getErrorString ();*/
        /*        exit (1);*/
        /*    }*/
        /*}*/

        {
            /* Load shaders */
            slrd::ShaderInfo shaderInfo;
            auto vshCode = loadSPIRVFromFile ("shaders/basic.vert.spv");
            auto fshCode = loadSPIRVFromFile ("shaders/basic.frag.spv");
            
            slrd::ShaderBytecode bytecodes[] = {
                vshCode, fshCode
            };
            shaderInfo.bytecodes = bytecodes;

            auto shader = m_device->createShader (shaderInfo);
            if (!shader) {
                std::cerr << "Failed to create shader: " << slrd::getErrorString ();
                exit (1);
            }

            /* RenderPass contains information about the type of attachments that 
             * are going to be bound during rendering
             *
             * In this case, we only bind color buffer, that belongs to the Swapchain. */
            slrd::RenderPassInfo rpInfo;
            slrd::RenderPassAttachment attachment;
            attachment.format = m_swapchain->getFormat ();
            attachment.loadOp = slrd::LOAD_OPERATION_CLEAR;
            attachment.storeOp = slrd::STORE_OPERATION_STORE;
            attachment.presentable = true;
            rpInfo.colorAttachments = { &attachment, 1 };

            m_renderPass = m_device->createRenderPass (rpInfo);
            if (!m_renderPass) {
                std::cerr << "Failed to create renderpass: " << slrd::getErrorString ();
                exit (1);
            }

            /* Create pipeline */
            slrd::GraphicsPipelineInfo plInfo;
            plInfo.shader = shader.get ();

            plInfo.vertexConfig.vertexBindings = Vertex::getBindingDescriptions ();
            plInfo.vertexConfig.attributeDescs = Vertex::getAttributeDescription ();
            plInfo.rasterizerConfig.cullMode = slrd::CULL_MODE_NONE;

            slrd::ColorBlendAttachment colorBlend;
            colorBlend.colorWriteMask = slrd::COLOR_MASK_RGBA;
            colorBlend.blendEnabled = false;

            plInfo.colorBlendConfig.attachments = { &colorBlend, 1 };

            m_pipeline = m_device->createGraphicsPipeline (plInfo);
            if (!m_pipeline) {
                std::cerr << slrd::getErrorString ();
                exit (1);
            }
        }

        /* Persistent map */
        /*(void)m_uniformBuffer->map ();*/

        /*m_pipeline->bindUniformBuffer (0, m_uniformBuffer);*/
    }

    int run () {
        bool isOpen = true;
        while (isOpen) {
            while (SDL_PollEvent (&m_event)) {
                switch (m_event.type) {
                    case SDL_QUIT:
                        isOpen = false;
                        break;
                    /*case SDL_WINDOWEVENT:*/
                    /*    if (m_event.window.type == SDL_WINDOWEVENT_RESIZED) {*/
                    /*        m_shouldRecreateSwapchain = true;*/
                    /*    }*/
                    /*    break;*/
                }
            }

            draw ();
        }

        return 0;
    }

    ~App () {
        m_device->waitIdle ();

        m_commandQueue = nullptr;
        m_fence = nullptr;
        m_renderPass = nullptr;
        m_triangleBuffer = nullptr;
        m_pipeline = nullptr;
        m_commandBuffer = nullptr;

        m_swapchain = nullptr;
        m_device = nullptr;

        SDL_DestroyWindow (m_window);
        SDL_Quit ();

        slrd::deinit ();
    }

    void recreateSwapchain () {
        m_device->waitIdle ();

        int w, h;
        SDL_GL_GetDrawableSize (m_window, &w, &h);

        std::cout << "m_swapchain->resize: resizing\n";
        int res = m_swapchain->resize (w, h);
        if (res) {
            throw std::runtime_error ("Failed to resize the swapchain");
        }
        m_fence = m_device->createFence (true);
    }

    void draw () {
        if (m_shouldRecreateSwapchain) {
            recreateSwapchain ();
            m_shouldRecreateSwapchain = false;
        }

        m_fence->wait ();
        m_fence->reset ();

        uint32_t imageIdx;
        if (auto result = m_swapchain->acquireNextImage (&imageIdx);
                result != slrd::SWAPCHAIN_RESULT_SUCCESS) {
            if (result == slrd::SWAPCHAIN_RESULT_NEEDS_RESIZE) {
                std::cout << ("m_swapchain->acquireNextImage: needs resize") << '\n';
                m_shouldRecreateSwapchain = true;
                return;
            }

            throw std::runtime_error (slrd::getErrorString ());
        }

        /* Get the next texture in the swapchain */
        auto texture_view = m_swapchain->getTextureView (imageIdx);
        /* Set the renderpass's texture view to this image
         *
         * (This, depending on the vulkan's version should either change the
         * framebuffer which is used by the renderpass, or should attach views
         * to the existing one if imageless-framebuffers are supported) */
        m_renderPass->setTextureView (0, texture_view);

        /* Reset the command buffer */
        m_commandBuffer->reset ();

        int w, h;
        SDL_GL_GetDrawableSize (m_window, &w, &h);
        std::cout << w << ' ' << h << '\n';

        m_commandBuffer->begin (); 
            slrd::RenderPassColorClearValue cv = { 0, 0, 0, 0 };
            slrd::RenderPassBeginInfo begInfo;
            begInfo.colorClearValues = { &cv, 1 };
            m_commandBuffer->beginRenderPass (m_renderPass.get (), begInfo);

            m_commandBuffer->bindGraphicsPipeline (m_pipeline.get ());

            m_commandBuffer->setViewport ({ 0, 0, (float)w, (float)h});
            m_commandBuffer->setScissor ({ 0, 0, (uint32_t)w, (uint32_t)h });

            m_commandBuffer->bindVertexBuffer (m_triangleBuffer.get (), 0, 0);
            m_commandBuffer->draw (3, 1);

            m_commandBuffer->endRenderPass ();
        m_commandBuffer->end ();

        slrd::SubmitInfo submitInfo;
        submitInfo.fence = m_fence.get ();

        slrd::ICommandBuffer *cmdBuffers[] = { m_commandBuffer };
        submitInfo.commandBuffers = cmdBuffers;

        m_commandQueue->submit (submitInfo);
        
        slrd::PresentInfo presentInfo;
        presentInfo.image = imageIdx;
        auto result = m_swapchain->present (presentInfo);
        if (result == slrd::SWAPCHAIN_RESULT_NEEDS_RESIZE) {
            std::cout << ("m_swapchain->present: needs resize") << '\n';
            m_shouldRecreateSwapchain = true;
            return;
        }
    }

    static std::vector<char> loadFileContents (const std::filesystem::path& path) {
        std::ifstream ifs (path.string (), std::ios::in | std::ios::binary | std::ios::ate);
        if (!ifs.is_open ())
            return {};

        std::ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        std::vector<char> bytes(fileSize);
        ifs.read(bytes.data(), fileSize);

        ifs.close ();

        return bytes;
    }

    static slrd::ShaderBytecode loadSPIRVFromFile (const std::filesystem::path& path) {
        std::ifstream ifs (path.string (), std::ios::in | std::ios::binary | std::ios::ate);
        if (!ifs.is_open ())
            return {};

        std::ifstream::pos_type fileSize = ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        if (fileSize & 3)
            throw std::runtime_error ("Size of the SPIRV is not aligned");

        std::vector<uint32_t> code(fileSize / 4);
        ifs.read(reinterpret_cast<char *>(code.data()), fileSize);

        ifs.close ();

        return slrd::ShaderBytecode (code.data (), code.size ());
    }
};

int main (void) {
    App app;
    return app.run ();
}
