#include <slrdframework/app.hpp>
#include <vulkan/vulkan.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <vector>
#include <iostream>

#include <slrd/slrd.hpp>
#include <slrd/platform/vulkan.hpp>

#include <imgui.h>
#include <backends/imgui_impl_sdl2.h>
#include <backends/imgui_impl_vulkan.h>

using DClock = std::chrono::high_resolution_clock;
using DDuration = std::chrono::duration<float, DClock::period>;
using DTimePoint = std::chrono::time_point<DClock>;

/* Maybe there is a better way to account for the clock resolution, idk */
template<typename Period>
static constexpr float toMillis (std::chrono::duration<float, Period> duration) {
    return duration.count () / Period::den;
}

Input::Input () {
    m_keystate = SDL_GetKeyboardState (&m_keystateLen);
}

bool Input::isKeyPressed (SDL_Scancode key) const {
    return m_keystate[key];
}

void Input::getMouse (int& x, int& y) const {
    SDL_GetMouseState (&x, &y);
}

void App::initImGUI (slrd::IRenderPass *renderpass) {
    m_imCtx = ImGui::CreateContext ();
    if (!ImGui_ImplSDL2_InitForVulkan (m_window)) {
        throw std::runtime_error ("Failed to initialize the ImGUI library");
    }
    auto vkapi = slrd::platform::vulkan::getVulkanAPIData ();

    /* FIXME: Migrate to imgui_impl_slrd */
    ImGui_ImplVulkan_InitInfo imInfo {};
    imInfo.Instance = vkapi->instance;
    imInfo.Device = slrd::platform::vulkan::getLogicalDevice (m_device.get ());
    imInfo.PhysicalDevice = slrd::platform::vulkan::getPhysicalDevice (m_device.get ());
    imInfo.Queue = slrd::platform::vulkan::getQueue (m_commandQueue.get ());
    imInfo.QueueFamily = slrd::platform::vulkan::getQueueFamily (m_commandQueue.get ());
    imInfo.RenderPass = slrd::platform::vulkan::getRenderPass (renderpass);
    imInfo.DescriptorPoolSize = 1024;
    imInfo.MinImageCount = 2;
    imInfo.ImageCount = 2;

    if (!ImGui_ImplVulkan_Init (&imInfo)) {
        throw std::runtime_error ("Failed to initialize the ImGUI library");
    }
}

void App::deinitImGUI () {
    ImGui_ImplVulkan_Shutdown ();
    ImGui_ImplSDL2_Shutdown ();
}

App::App (const AppInit& init) {
    if (SDL_Init (SDL_INIT_EVENTS | SDL_INIT_VIDEO)) {
        throw std::runtime_error ("Failed to initialize SDL2");
    }

    SDL_Vulkan_LoadLibrary (nullptr);

    m_window = SDL_CreateWindow (init.name.c_str (), 0, 0, init.width, init.height, SDL_WINDOW_VULKAN);
    if (!m_window) {
        throw std::runtime_error ("Failed to initialize SDL2 window");
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
    config.appName = init.name;
    config.devName = "slayer";
    config.engineName = "slrd";
    config.appVersion = { 1, 0, 0 }; 
    config.engineVersion = { 0, 0, 1 };
    config.instanceExtensions = instanceExtensions;
    config.debug = true;

    auto apis = slrd::querySupportedAPIs ();
    if (!(apis & slrd::API_VULKAN)) {
        throw std::runtime_error ("No vulkan support");
    }

    if (slrd::init (slrd::API_VULKAN, config)) {
        throw std::runtime_error ("Failed to initialize vulkan");
    }

    {
        static const char *devext[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        slrd::DeviceConfig devconf;
        devconf.deviceExtensions = devext;
        devconf.debug = true;
        m_device = slrd::createDevice (devconf);
        if (!m_device) {
            throw std::runtime_error ("Failed to create device");
            exit (1);
        }
    }

    /* Create the window surface */
    {
        auto *data = slrd::platform::vulkan::getVulkanAPIData ();

        VkSurfaceKHR vksurface;
        if (!SDL_Vulkan_CreateSurface (m_window, data->instance, &vksurface)) {
            throw std::runtime_error ("Failed to create a window surface");
        }

        slrd::SurfaceInfo info;
        info.apiData.ptr = vksurface;
        auto surface = slrd::createSurface (info);
        if (!surface) {
            throw std::runtime_error ("Failed to create a window surface");
        }

        /* Create the swapchain with the surface */
        slrd::SwapchainInfo swpInfo;
        swpInfo.surface = surface.get ();
        swpInfo.requireVSync = init.requireVSync;
        swpInfo.width = 800;
        swpInfo.height = 600;
        swpInfo.requestedImages = 2;

        m_swapchain = m_device->createSwapchain (swpInfo);
        if (!m_swapchain) {
            throw std::runtime_error ("Failed to create the swapchain");
            
        }

        m_fence = m_device->createFence (true);
        if (!m_fence) {
            throw std::runtime_error ("Failed to create the fence");
        }
    }

    slrd::CommandQueueInfo cqInfo {};
    cqInfo.flags = slrd::COMMAND_QUEUE_GRAPHICS;
    m_commandQueue = m_device->createCommandQueue (cqInfo);
    if (!m_commandQueue) {
        throw std::runtime_error ("Failed to create the command queue");
    }

    slrd::CommandPoolInfo pInfo {};
    pInfo.queue = m_commandQueue.get ();
    m_commandPool = m_device->createCommandPool (pInfo);
    if (!m_commandQueue) {
        throw std::runtime_error ("Failed to create the command pool");
    }

    createDepthResource (init.width, init.height);

    m_width = init.width;
    m_height = init.height;

    m_input = std::unique_ptr<Input> (new Input ());

    m_epoch = std::chrono::high_resolution_clock::now ();

    m_profiler = std::make_unique<Profiler> ();
}

void App::createDepthResource (uint32_t width, uint32_t height) {
    slrd::TextureInfo ti;
    ti.type = slrd::TEXTURE_TYPE_2D;
    ti.usage = slrd::TEXTURE_USAGE_DEPTH | slrd::TEXTURE_USAGE_STENCIL;
    ti.width = width;
    ti.height = height;
    ti.format = slrd::FORMAT_D24UNORMS8UINT;

    m_depth = m_device->createTexture (ti);
    if (!m_depth) {
        throw std::runtime_error ("Failed to create depth resource");
    }

    slrd::TextureViewInfo viewInfo;
    viewInfo.mipLevels = 1;
    viewInfo.arrayLayers = 1;
    viewInfo.aspect = slrd::TEXTURE_ASPECT_DEPTH;
    m_depthView = m_depth->createTextureView (viewInfo);
    if (!m_depthView) {
        throw std::runtime_error ("Failed to create depth resource view");
    }
}

slrd::ITextureView *App::nextFrame () {
    if (m_shouldRecreateSwapchain) {
        recreateSwapchain ();
        m_shouldRecreateSwapchain = false;
    }

    m_fence->wait ();
    m_fence->reset ();

    uint32_t image;
    auto res = m_swapchain->acquireNextImage (&image);
    while (res == slrd::SWAPCHAIN_RESULT_NEEDS_RESIZE) {
        m_shouldRecreateSwapchain = true;
        return nullptr;
    }

    if (res != slrd::SWAPCHAIN_RESULT_SUCCESS) {
        throw std::runtime_error ("Failed to get next frame");
    }

    m_currentImage = image;

    return m_swapchain->getTextureView (image);
}
void App::recreateSwapchain () {
    m_device->waitIdle ();

    int w, h;
    SDL_GL_GetDrawableSize (m_window, &w, &h);

    std::cout << "m_swapchain->resize: resizing\n";
    int res = m_swapchain->resize (w, h);
    if (res) {
        throw std::runtime_error ("Failed to resize the swapchain");
    }
    m_fence = m_device->createFence (true);

    m_width = w;
    m_height = h;

    createDepthResource (w, h);
    //usleep (100);

    m_recreatedSwapchain = true;
}

void App::present () {
    slrd::PresentInfo presentInfo;
    presentInfo.image = m_currentImage;

    if (m_swapchain->present (presentInfo) == slrd::SWAPCHAIN_RESULT_NEEDS_RESIZE) {
        m_shouldRecreateSwapchain = true;
        return;
    }
}

int App::run () {
    m_prevTime = m_epoch;

    while (!m_shouldClose) {
        DTimePoint tp = DClock::now ();

        DDuration dur = tp - m_prevTime;
        m_delta = toMillis (dur);

        while (SDL_PollEvent (&m_event)) {
            if (m_imCtx)
                ImGui_ImplSDL2_ProcessEvent (&m_event);

            switch (m_event.type) {
                case SDL_QUIT:
                    m_shouldClose = true;
                    break;
                case SDL_KEYUP:
                    if (m_event.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
                        m_controlsDisabled = !m_controlsDisabled;
                    }
                    break;
            }
        }

        if (m_imCtx) {
            ImGui_ImplVulkan_NewFrame ();
            ImGui_ImplSDL2_NewFrame ();
        }

        draw ();

        m_prevTime = tp;
    }

    return 0;
}

void App::newImGuiFrame () {
    ImGui::NewFrame ();
}

void App::renderImGui (slrd::ICommandBuffer *cmdBuffer) {
    ImGui::Render ();
    auto drawData = ImGui::GetDrawData ();

    auto cmd = slrd::platform::vulkan::getCommandBuffer (cmdBuffer);
    ImGui_ImplVulkan_RenderDrawData (drawData, cmd);
}

float App::getElapsedTime () const {
    auto now = DClock::now ();
    DDuration dur = now - m_epoch;

    return toMillis (dur);
}

float App::getDeltaTime () const {
    return m_delta;
}

App::~App () {
    m_commandQueue->wait ();

    m_surface = nullptr;
    m_swapchain = nullptr;
    m_depth = nullptr;
    m_depthView = nullptr;
    m_commandQueue = nullptr;
    m_commandPool = nullptr;
    m_fence = nullptr;

    if (m_imCtx)
        deinitImGUI ();

    m_device->waitIdle ();
    m_device = nullptr;

    if (m_window)
        SDL_DestroyWindow (m_window);

    SDL_Vulkan_UnloadLibrary ();
    SDL_Quit ();

    slrd::deinit ();
}
