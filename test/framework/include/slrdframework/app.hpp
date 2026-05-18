#ifndef __APP_HPP__
#define __APP_HPP__

#include <SDL2/SDL.h>

#include <chrono>
#include <slrd/api.hpp>
#include <slrd/types.hpp>
#include <slrd/pipeline.hpp>

#include <imgui.h>

#include "profiler.hpp"

class Input {
private:
    const uint8_t *m_keystate = nullptr;
    int m_keystateLen = 0;

    Input ();

public:
    Input (Input&) = delete;
    Input& operator= (Input&) = delete;

    bool isKeyPressed (SDL_Scancode key) const;
    void getMouse (int& x, int& y) const;

    friend class App;
};

class App {
private:
    void createDepthResource (uint32_t, uint32_t);
    void recreateSwapchain ();

    uint32_t m_width;
    uint32_t m_height;

    float m_delta;
    std::chrono::time_point<std::chrono::high_resolution_clock> m_prevTime;

    /* The point from which we count the milliseconds passed */
    std::chrono::time_point<std::chrono::high_resolution_clock> m_epoch;

    mutable bool m_recreatedSwapchain = true;

    ImGuiContext *m_imCtx = nullptr;
    void deinitImGUI ();

protected:
    bool m_controlsDisabled = true;

    std::unique_ptr<Profiler> m_profiler;

    /* ImGUI stuff */
    void initImGUI (slrd::IRenderPass *renderPass);

    slrd::Ref<slrd::IDevice> m_device;
    slrd::Ref<slrd::ISurface> m_surface;
    slrd::Ref<slrd::ISwapchain> m_swapchain;
    slrd::Ref<slrd::IFence> m_fence;

    slrd::Ref<slrd::ICommandQueue> m_commandQueue;

    slrd::Ref<slrd::ITexture> m_depth;
    slrd::Ref<slrd::ITextureView> m_depthView;

    bool m_shouldClose = false;
    bool m_shouldRecreateSwapchain = false;
    uint32_t m_currentImage = 0;

    SDL_Window *m_window;
    SDL_Event   m_event;

    inline uint32_t getWidth () const {
        return m_width;
    }
    inline uint32_t getHeight () const {
        return m_height;
    }

    float getElapsedTime () const;
    float getDeltaTime () const;

    void poolEvents ();
    virtual void draw () = 0;

    /* Get the next image in the swapchain
     * (resizes the swapchain, if needed) */
    slrd::ITextureView *nextFrame ();
    /* Present the image */
    void present ();

    slrd::IFence *getCurrentFence () {
        return m_fence.get ();
    }

    struct AppInit {
        std::string name = "Test";
        slrd::API api = slrd::API_VULKAN;

        uint32_t width = 800;
        uint32_t height = 600;

        bool requireVSync = true;
    };
    App (const AppInit& init);

    bool wasSwapchainRecreated () const {
        bool res = m_recreatedSwapchain;
        m_recreatedSwapchain = false;
        return res;
    }

    void newImGuiFrame ();
    void renderImGui (slrd::ICommandBuffer *cmdBuffer);

    std::unique_ptr<Input> m_input;

public:
    App () = delete;
    ~App ();

    int run ();

    App (const App&) = delete;
    App& operator= (const App&) = delete;
};

#endif /* #define __APP_HPP__ */
