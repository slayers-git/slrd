#include "slrd/error.hpp"
#include "slrd/pipeline.hpp"
#include "slrd/uniformset.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>
#include <array>

#include <chrono>

#include <slrd/api.hpp>
#include <slrd/slrd.hpp>
#include <slrd/device.hpp>

#include <slrd/platform/vulkan.hpp>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <unordered_map>
#include <vector>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/packing.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION 1
#include <stb/stb_image.h>

#include <slrd/uniformupdater.hpp>

/* The uniform buffer for the cube */
struct ViewProjectionUB {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 mergedVP;
};

struct Vertex {
    glm::vec3 position;
    glm::vec2 color;

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
                .format   = slrd::FORMAT_RG32_SFLOAT,
            }
        };

        return attrInfo;
    }
};

static const Vertex s_vertices[] = {
    /* a */
    { { -1,  1,  1 }, { 0, 1 } },
    { { -1,  1,  1 }, { 0, 0 } },
    { { -1,  1,  1 }, { 1, 1 } },

    /* b */
    { {  1,  1,  1 }, { 1, 1 } },
    { {  1,  1,  1 }, { 1, 0 } },
    { {  1,  1,  1 }, { 0, 1 } },

    /* c */
    { {  1, -1,  1 }, { 1, 0 } },
    { {  1, -1,  1 }, { 0, 0 } },
    { {  1, -1,  1 }, { 0, 0 } },

    /* d */
    { { -1, -1,  1 }, { 0, 0 } },
    { { -1, -1,  1 }, { 1, 0 } },
    { { -1, -1,  1 }, { 1, 0 } },

    /* e */
    { { -1,  1, -1 }, { 1, 1 } },
    { { -1,  1, -1 }, { 0, 1 } },
    { { -1,  1, -1 }, { 0, 1 } },

    /* f */
    { {  1,  1, -1 }, { 0, 1 } },
    { {  1,  1, -1 }, { 1, 1 } },
    { {  1,  1, -1 }, { 1, 1 } },

    /* g */
    { {  1, -1, -1 }, { 0, 0 } },
    { {  1, -1, -1 }, { 0, 1 } },
    { {  1, -1, -1 }, { 1, 0 } },

    /* h */
    { { -1, -1, -1 }, { 1, 0 } },
    { { -1, -1, -1 }, { 1, 1 } },
    { { -1, -1, -1 }, { 0, 0 } }
};

#define POLY(__A, __B, __C, __D) \
    (__A), (__B), (__C), (__C), (__D), (__A)

/* the indices for cube faces */
static uint16_t s_indices[] = {
    /* front */
    /* 0, 3, 6, 6, 9, 0, */
    POLY (6, 3, 0, 9),
    /* left */
    /* 11, 23, 14, 14, 2, 11, */
    POLY (14, 23, 11, 2),
    /* right */
    /* 5, 17, 20, 20, 8, 5, */
    POLY (20, 17, 5, 8),
    /* back */
    /* 21, 18, 15, 15, 12, 21, */
    POLY (15, 18, 21, 12),
    /* down */
    /* 10, 7, 19, 19, 22, 10, */
    POLY (19, 7, 10, 22),
    /* up */
    /* 13, 16, 4, 4, 1, 13 */
    POLY (4, 16, 13, 1)
};


class Profiler {
private:
    struct ProfileData {
        /* Scope name */
        const char *name;
        float duration = 0;

        std::chrono::time_point<std::chrono::high_resolution_clock> tp {};
    };

    std::unordered_map<const char *, ProfileData> m_scopes;

public:
    Profiler () = default;

    void newFrame () {
        m_scopes.clear ();
        startScope ("Frame");
    }

    void endFrame () {
        endScope ("Frame");
    }

    void printData () {
        std::cout << "Profiler frame: \n";
        for (auto scope : m_scopes) {
            auto& data = scope.second;
            std::cout << std::format ("- {}: {:.6f}ms\n", data.name, data.duration);
        }
    }

    void startScope (const char *scopeName) {
        if (m_scopes.find (scopeName) == m_scopes.end ()) {
            m_scopes.emplace (scopeName, ProfileData{ scopeName, 0,
                    std::chrono::high_resolution_clock::now () });
        } else {
            m_scopes[scopeName].tp = std::chrono::high_resolution_clock::now ();
        }
    }

    void endScope (const char *scopeName) {
        if (m_scopes.find (scopeName) == m_scopes.end ())
            throw std::runtime_error ("invalid scope");
        auto& scope = m_scopes[scopeName];
        std::chrono::duration<float, std::nano> dur = std::chrono::high_resolution_clock::now () -
            scope.tp;

        scope.duration += dur.count () / 1'000'000'000;
    }
};
Profiler profiler;

struct App {
    slrd::Ref<slrd::IDevice> m_device;
    slrd::Ref<slrd::ISwapchain> m_swapchain;

    /* Pointer to the memory for the vp UB */
    void *m_vpBufferMap;
    
    ViewProjectionUB m_currentVP;

    slrd::Ref<slrd::IRenderPass> m_renderPass;
    slrd::Ref<slrd::IPipeline> m_pipeline;

    /* This program uses one inflight frame */

    slrd::Ref<slrd::IFence> m_fence;

    slrd::Ref<slrd::IBuffer> m_vpBuffer;

    slrd::Ref<slrd::IBuffer> m_vertexBuffer;
    slrd::Ref<slrd::IBuffer> m_indexBuffer;

    slrd::Ref<slrd::ICommandQueue> m_commandQueue;
    slrd::ICommandBuffer *m_commandBuffer;

    slrd::Ref<slrd::IUniformSet> m_uniformSet;

    /* depth attachment */
    slrd::Ref<slrd::ITexture> m_depthTexture;
    slrd::Ref<slrd::ITextureView> m_depthTextureView;

    slrd::Ref<slrd::ITexture> m_texture;
    slrd::Ref<slrd::ITextureView> m_textureView;
    slrd::Ref<slrd::ISampler> m_sampler;

    bool m_shouldRecreateSwapchain = false;

    SDL_Window *m_window;
    SDL_Event  m_event;

    float m_delta = 0.f;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_prevTime {};

    slrd::Ref<slrd::ITexture> createTextureFromImage (
            const std::filesystem::path& path,
            slrd::Ref<slrd::ITextureView> *texView,
            std::string_view name = "") {
        if (!std::filesystem::exists (path)) {
            return nullptr;
        }

        int w, h, c;
        uint8_t *const udata = stbi_load (path.c_str (), &w, &h, &c, 4);

        auto oneTime = m_commandQueue->getCommandBuffer ();
        if (!oneTime) {
            std::cout << "Failed to create one time command buffer\n";
            return nullptr;
        }

        slrd::BufferInfo bufInfo;
        bufInfo.usage = 0;
        bufInfo.gpu = true;
        bufInfo.coherent = true;
        bufInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_SRC;
        bufInfo.size = w * h * 4;
        bufInfo.name = "staging_buf";
        slrd::Ref<slrd::IBuffer> stagingBuffer = m_device->createBuffer (bufInfo);
        if (!stagingBuffer) {
            std::cout << "Failed to create one time staging buffer\n";
            return nullptr;
        }

        void *map = stagingBuffer->map ();
        if (!map) {
            std::cout << "Failed to create a staging buffer map\n";
            return nullptr;
        }
        memcpy (map, udata, w * h * 4);
        free (udata);
        stagingBuffer->unmap ();

        slrd::TextureInfo texInfo;
        texInfo.name = name;
        texInfo.type = slrd::TEXTURE_TYPE_2D;
        texInfo.width = w;
        texInfo.height = h;
        texInfo.usage = slrd::TEXTURE_USAGE_SAMPLED | slrd::TEXTURE_USAGE_TRANSFER_DST;
        texInfo.format = slrd::FORMAT_RGBA8_UNORM;
        texInfo.tiling = slrd::TEXTURE_TILING_OPTIMAL;

        slrd::Ref<slrd::ITexture> texture = m_device->createTexture (texInfo);
        if (!texture) {
            return nullptr;
        }

        if (!texView)
            return texture;

        oneTime->begin ();

        slrd::BufferTextureCopyInfo bufTexCopyInfo;
        slrd::BufferTextureRegion region;

        region.rect = slrd::Rect3D<uint32_t>{
            0, 0, 0, (uint32_t)w, (uint32_t)h, 1
        };
        region.rows = w;
        region.height = h;
        region.textureViewInfo.arrayLayer = 0;
        region.textureViewInfo.arrayLayers = 1;
        region.textureViewInfo.mipLevel = 0;
        region.textureViewInfo.mipLevels = 1;

        bufTexCopyInfo.buffer = stagingBuffer.get ();
        bufTexCopyInfo.texture = texture.get ();
        bufTexCopyInfo.regions = { &region, 1 };

        slrd::TextureBarrierInfo tbInfo;
        tbInfo.texture = texture.get ();
        tbInfo.currentTextureLayout = slrd::TEXTURE_LAYOUT_UNDEFINED;
        tbInfo.newTextureLayout = slrd::TEXTURE_LAYOUT_TRANSFER_DST;
        tbInfo.viewInfo.mipLevels = 1;
        tbInfo.viewInfo.mipLevel  = 0;
        tbInfo.viewInfo.aspect = slrd::TEXTURE_ASPECT_COLOR;
        tbInfo.viewInfo.arrayLayer = 0;
        tbInfo.viewInfo.arrayLayers = 1;

        oneTime->pipelineTextureBarrier (tbInfo);
        oneTime->copyBufferToImage (bufTexCopyInfo);

        tbInfo.currentTextureLayout = slrd::TEXTURE_LAYOUT_TRANSFER_DST;
        tbInfo.newTextureLayout = slrd::TEXTURE_LAYOUT_SHADER_READ_ONLY;
        oneTime->pipelineTextureBarrier (tbInfo);

        oneTime->end ();

        slrd::SubmitInfo info;
        info.commandBuffers = { &oneTime, 1 };
        int res = m_commandQueue->submit (info);
        if (res) {
            std::cout << "Failed to submit one time command buffer\n";
            return nullptr;
        }

        /* Wait for the operations to complete, before deleting the buffer */
        m_commandQueue->wait ();

        slrd::TextureViewInfo viewInfo;
        viewInfo.mipLevels = 1;
        viewInfo.arrayLayers = 1;
        std::string view_name = std::string (name) + "_view";
        viewInfo.name = view_name;

        slrd::Ref<slrd::ITextureView> view = texture->createTextureView (viewInfo);
        if (!view) {
            return nullptr;
        }

        *texView = view;
        return texture;
    }

    App () {
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
        config.appName = "Test SLRD";
        config.devName = "slayer";
        config.engineName = "slrd";
        config.appVersion = { 1, 0, 0 }; 
        config.engineVersion = { 0, 0, 1 };
        config.instanceExtensions = instanceExtensions;

        config.debug = true;
        config.debugFlags = slrd::API_DEBUG_FLAG_NAMES | slrd::API_DEBUG_RESOURCE_PROFILER;

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
            devconf.deviceExtensions = devext;
            devconf.debug = true;
            devconf.debugFlags = slrd::DEVICE_DEBUG_FLAG_RESOURCE_PROFILER |
                slrd::DEVICE_DEBUG_FLAG_API_RESOURCE_PROFILER;
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
            swpInfo.requireVSync = true;
            swpInfo.width = 800;
            swpInfo.height = 600;
            swpInfo.requestedImages = 4;

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

        createDepth (800, 600);

        init ();
    }

    slrd::Ref<slrd::IBuffer> createBufferWithData (
            slrd::BufferUsageFlags usage,
            void *data,
            size_t size,
            std::string_view name = "") {
        slrd::Ref<slrd::IBuffer> result, stagingBuffer;
        auto oneTimeBuffer = m_commandQueue->getCommandBuffer (true);
        if (!oneTimeBuffer) {
            std::cout << "Failed to create a one time buffer\n";
            return nullptr;
        }

        slrd::BufferInfo stagingInfo {};
        stagingInfo.usage = 0;
        stagingInfo.coherent = true;
        stagingInfo.gpu = true;
        stagingInfo.size = size;
        stagingInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_SRC;
        stagingInfo.name = "staging_buf";

        stagingBuffer = m_device->createBuffer (stagingInfo);
        if (!stagingBuffer) {
            return nullptr;
        }

        void *map = stagingBuffer->map ();
        if (!map) {
            return nullptr;
        }

        memcpy (map, data, size);

        slrd::BufferInfo resultInfo;
        resultInfo.usage = usage;
        resultInfo.coherent = false;
        resultInfo.gpu = true;
        resultInfo.size = size;
        resultInfo.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;
        resultInfo.name = name;

        result = m_device->createBuffer (resultInfo);
        if (!result) {
            return nullptr;
        }

        oneTimeBuffer->begin ();

        slrd::BufferCopyInfo copyInfo;
        copyInfo.srcBuffer = stagingBuffer.get ();
        copyInfo.dstBuffer = result.get ();
        copyInfo.size = size;
        oneTimeBuffer->copyBuffer (copyInfo);

        oneTimeBuffer->end ();

        slrd::SubmitInfo submitInfo {};
        submitInfo.commandBuffers = { &oneTimeBuffer, 1 };
        if (m_commandQueue->submit (submitInfo)) {
            return nullptr;
        }

        m_commandQueue->wait ();

        return result;
    }

    void init () {
        /* Initialize the cube */
        {
            slrd::BufferInfo bi;
            bi.usage = slrd::BUFFER_USAGE_VERTEX_BUFFER;
            bi.coherent = false;
            bi.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;
            bi.size = sizeof (s_vertices);

            m_vertexBuffer = createBufferWithData (slrd::BUFFER_USAGE_VERTEX_BUFFER,
                    (void *)s_vertices, sizeof (s_vertices), "cube_vert");
            if (!m_vertexBuffer) {
                std::cerr << "Failed to create buffer\n";
                exit (1);
            }

            m_indexBuffer = createBufferWithData (slrd::BUFFER_USAGE_INDEX_BUFFER,
                    (void *)s_indices, sizeof (s_indices), "cube_idx");
            if (!m_indexBuffer) {
                std::cerr << "Failed to create buffer\n";
                exit (1);
            }
        }

        /* Initialize the VP Uniform Buffer */
        {
            slrd::BufferInfo bi;
            bi.usage = slrd::BUFFER_USAGE_UNIFORM_BUFFER;
            bi.properties = slrd::BUFFER_PROPERTY_TRANSFER_DST;
            bi.coherent = true;
            bi.size = sizeof (ViewProjectionUB);

            m_vpBuffer = m_device->createBuffer (bi);
            if (!m_vpBuffer) {
                std::cerr << "Failed to create uniform buffer: " << slrd::getErrorString ();
                exit (1);
            }

            m_vpBufferMap = m_vpBuffer->map ();
            if (!m_vpBufferMap) {
                std::cerr << "Failed to map the UniformBuffer\n";
                exit (1);
            }
        }

        {
            /* Load shaders */
            slrd::ShaderInfo shaderInfo;
            auto vshCode = loadSPIRVFromFile ("shaders/cube.vert.spv");
            auto fshCode = loadSPIRVFromFile ("shaders/cube.frag.spv");
            
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
            slrd::RenderPassAttachment attachment, depth;
            attachment.format = m_swapchain->getFormat ();
            attachment.loadOp = slrd::LOAD_OPERATION_CLEAR;
            attachment.storeOp = slrd::STORE_OPERATION_STORE;
            attachment.presentable = true;

            depth.format = slrd::FORMAT_D24UNORMS8UINT;
            depth.loadOp = slrd::LOAD_OPERATION_CLEAR;
            depth.storeOp = slrd::STORE_OPERATION_DONT_CARE;
            depth.presentable = false;
            rpInfo.colorAttachments = { &attachment, 1 };
            rpInfo.depthAttachment = depth;

            m_renderPass = m_device->createRenderPass (rpInfo);
            if (!m_renderPass) {
                std::cerr << "Failed to create renderpass: " << slrd::getErrorString ();
                exit (1);
            }
            m_renderPass->setDepthView (m_depthTextureView.get ());

            /* Create pipeline */
            slrd::GraphicsPipelineInfo plInfo;
            plInfo.shader = shader.get ();

            plInfo.vertexConfig.vertexBindings = Vertex::getBindingDescriptions ();
            plInfo.vertexConfig.attributeDescs = Vertex::getAttributeDescription ();
            plInfo.rasterizerConfig.cullMode = slrd::CULL_MODE_BACK;
            plInfo.depthStencilConfig.depthFormat = slrd::FORMAT_D24UNORMS8UINT;
            plInfo.depthStencilConfig.depthTestEnabled = true;
            plInfo.depthStencilConfig.depthWriteEnabled = true;
            plInfo.depthStencilConfig.compareOperator = slrd::COMPARE_OPERATOR_LESS;

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

        m_texture = createTextureFromImage ("res/test_texture.png", &m_textureView, "main_tex");
        if (!m_texture) {
            exit (1);
        }

        /* Use the default initialized one */
        slrd::SamplerInfo samplerInfo;
        m_sampler = m_device->createSampler (samplerInfo);
        if (!m_sampler) {
            std::cerr << "Failed to create a sampler\n";
            exit (1);
        }

        m_uniformSet = m_pipeline->allocateUniformSet (0);
        if (!m_uniformSet) {
            std::cerr << "Failed to allocate a uniform set\n";
            exit (1);
        }

        slrd::UniformUpdater updater;
        updater.updateUniformBuffer (0, m_vpBuffer.get (), sizeof (ViewProjectionUB)).
            updateCombinedTexture (1, m_textureView.get (), m_sampler.get (),
                    slrd::TEXTURE_LAYOUT_SHADER_READ_ONLY);

        m_uniformSet->updateUniforms (updater.get ());

        m_currentVP.proj = glm::perspective <float> (90, (float)800/600, 0.1f, 100.f);
        m_currentVP.view = glm::identity<glm::mat4> ();
        m_currentVP.mergedVP = m_currentVP.proj * m_currentVP.view;
        memcpy (m_vpBufferMap, &m_currentVP, sizeof (m_currentVP));

        m_prevTime = std::chrono::high_resolution_clock ().now ();

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

                    case SDL_KEYUP:
                        if (m_event.key.keysym.sym == SDLK_p) {
                            profiler.printData ();
                        }
                        break;
                }
            }
            
            {
                const auto now = std::chrono::high_resolution_clock ().now ();
                std::chrono::duration<float, std::nano> diff = now - m_prevTime;

                m_delta = diff.count () / 1'000'000'000;
                m_prevTime = now;
            }

            profiler.newFrame ();
            draw ();
            profiler.endFrame ();
        }

        return 0;
    }

    void createDepth (uint32_t width, uint32_t height) {
        slrd::TextureInfo ti;
        ti.type = slrd::TEXTURE_TYPE_2D;
        ti.usage = slrd::TEXTURE_USAGE_DEPTH | slrd::TEXTURE_USAGE_STENCIL;
        ti.width = width;
        ti.height = height;
        ti.format = slrd::FORMAT_D24UNORMS8UINT;
        ti.name = "depth_texture";

        m_depthTexture = m_device->createTexture (ti);
        if (!m_depthTexture) {
            std::cerr << "Failed to create the depth resource\n";
            exit (1);
        }

        slrd::TextureViewInfo viewInfo;
        viewInfo.mipLevels = 1;
        viewInfo.arrayLayers = 1;
        viewInfo.aspect = slrd::TEXTURE_ASPECT_DEPTH;
        viewInfo.name = "depth_view";
        m_depthTextureView = m_depthTexture->createTextureView (viewInfo);
    }

    ~App () {
        m_device->waitIdle ();
        SDL_DestroyWindow (m_window);
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

        m_currentVP.proj = glm::perspective <float> (90, (float)w/h, 0.1f, 100.f);
        m_currentVP.mergedVP = m_currentVP.proj * m_currentVP.view;

        memcpy (m_vpBufferMap, &m_currentVP, sizeof (m_currentVP));

        createDepth (w, h);
        m_renderPass->setDepthView (m_depthTextureView.get ());
    }

    void draw () {
        m_fence->wait ();

        if (m_shouldRecreateSwapchain) {
            recreateSwapchain ();
            m_shouldRecreateSwapchain = false;
        }

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

        m_fence->reset ();

        /* Get the next image in the swapchain */
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

        /*auto n = std::chrono::high_resolution_clock::now ().time_since_epoch ();*/
        /*float t = std::chrono::duration_cast<std::chrono::milliseconds> (n).count ();*/

        static float angle = 0;
        angle += m_delta;

        glm::mat4 model = glm::identity<glm::mat4> ();
        model = glm::translate (model, { 0, 0, 2 });
        model = glm::rotate (model, angle, { 1, 0.2, 0 });

        glm::mat4 transform = model;
        
        slrd::RenderPassColorClearValue cv = { 0.f, 0.f, 0.f, 0.f };
        slrd::RenderPassBeginInfo begInfo;
        begInfo.colorClearValues = { &cv, 1 };

        profiler.startScope ("CommandBufferRecording");
        m_commandBuffer->begin (); 
            m_commandBuffer->beginRenderPass (m_renderPass.get (), begInfo);

            m_commandBuffer->bindGraphicsPipeline (m_pipeline.get ());

            m_commandBuffer->setViewport ({ 0, 0, (float)w, (float)h});
            m_commandBuffer->setScissor ({ 0, 0, (uint32_t)w, (uint32_t)h });

            m_commandBuffer->pushConstant (
                    { (uint8_t *)glm::value_ptr (transform), 64 }, slrd::STAGE_VERTEX);

            /* Bind the sets before we draw */
            slrd::IUniformSet* sets[] = { m_uniformSet.get () };
            profiler.startScope ("BindSets");
            m_commandBuffer->bindSets (sets);
            profiler.endScope ("BindSets");

            m_commandBuffer->bindVertexBuffer (m_vertexBuffer.get (), 0, 0);
            m_commandBuffer->bindIndexBuffer (m_indexBuffer.get (), slrd::INDEX_TYPE_UINT16);
            m_commandBuffer->drawIndexed (36, 1);

            m_commandBuffer->endRenderPass ();
        m_commandBuffer->end ();
        profiler.endScope ("CommandBufferRecording");

        static int frame = 0;

        slrd::SubmitInfo submitInfo;
        submitInfo.fence = m_fence.get ();

        slrd::ICommandBuffer *cmdBuffers[] = { m_commandBuffer };
        submitInfo.commandBuffers = cmdBuffers;

        profiler.startScope ("Submit");
        if (m_commandQueue->submit (submitInfo)) {
            throw std::runtime_error ("Queue submission failed");
        }
        profiler.endScope ("Submit");
        
        profiler.startScope ("Present");
        slrd::PresentInfo presentInfo;
        presentInfo.image = imageIdx;
        auto result = m_swapchain->present (presentInfo);
        profiler.endScope ("Present");

        if (result == slrd::SWAPCHAIN_RESULT_NEEDS_RESIZE) {
            std::cout << ("m_swapchain->present: needs resize") << '\n';
        }
        if (result == slrd::SWAPCHAIN_RESULT_OTHER) {
            throw std::runtime_error ("Queue presentation failed");
        }
        std::cout << "FRAME SURVIVED: " << frame++ << '\n';
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
    if (SDL_Init (SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        std::cerr << "Failed to initilize SDL.\n";
        exit (1);
    }

    SDL_Vulkan_LoadLibrary (nullptr);

    int res;
    {
        App app;
        res = app.run ();
    }

    SDL_Quit ();
    slrd::deinit ();

    return res;
}
