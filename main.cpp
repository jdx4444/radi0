#include <stdio.h>
#include <SDL.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "modules/BluetoothAudioManager.h"
#include "modules/Sprite.h"
#include "modules/UI.h"

// Define an 8:3 virtual coordinate system (80×30)
static const float VIRTUAL_WIDTH = 80.0f;
static const float VIRTUAL_HEIGHT = 30.0f;

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#else
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    // Get current screen resolution
    SDL_DisplayMode DM;
    if (SDL_GetCurrentDisplayMode(0, &DM) != 0)
    {
        printf("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        return -1;
    }
    int window_width = DM.w;   // e.g., 1280
    int window_height = DM.h;  // e.g., 480

    // Compute scale factors so that our 80×30 virtual space fits exactly
    float scale_x = static_cast<float>(window_width) / VIRTUAL_WIDTH;
    float scale_y = static_cast<float>(window_height) / VIRTUAL_HEIGHT;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    // Create a borderless fullscreen window at the detected resolution
    SDL_WindowFlags window_flags = (SDL_WindowFlags)(
        SDL_WINDOW_OPENGL |
        SDL_WINDOW_ALLOW_HIGHDPI |
        SDL_WINDOW_BORDERLESS |
        SDL_WINDOW_FULLSCREEN_DESKTOP
    );
    SDL_Window* window = SDL_CreateWindow("Retro Car Head Unit",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          window_width, window_height,
                                          window_flags);
    if (!window)
    {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context)
    {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();

    // Retro style: green text on black background
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.Colors[ImGuiCol_WindowBg]      = ImVec4(0, 0, 0, 1);
    style.Colors[ImGuiCol_Text]          = ImVec4(0, 1, 0, 1);
    style.Colors[ImGuiCol_Border]        = ImVec4(0, 1, 0, 1);
    style.Colors[ImGuiCol_FrameBg]       = ImVec4(0, 0, 0, 1);
    style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0, 1, 0, 1);

    ImVec4 clear_color = ImVec4(0, 0, 0, 1);

    // Initialize ImGui backends
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize BluetoothAudioManager (DBus enabled)
    BluetoothAudioManager audioManager;
    if (!audioManager.Initialize())
    {
        printf("Failed to initialize BluetoothAudioManager.\n");
        return -1;
    }
    
    // In mock mode, add dummy playlist entries.
#ifdef NO_DBUS
    audioManager.AddToPlaylist("Track1.mp3", 10.0f);
    audioManager.AddToPlaylist("Track2.mp3", 200.0f);
#endif

    audioManager.Play();

    // Initialize Sprite and UI modules
    Sprite sprite;
    sprite.Initialize(scale_x, scale_y);
    UI ui;
    ui.Initialize();

    bool done = false;
    while (!done)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
                done = true;
            // Keyboard controls
            if (event.type == SDL_KEYDOWN)
            {
                // Map the "e" key to exit the program
                if (event.key.keysym.sym == SDLK_e)
                {
                    done = true;
                }
                switch (event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        if (audioManager.GetState() == PlaybackState::Playing)
                        {
                            audioManager.Pause();
                        }
                        else if (audioManager.GetState() == PlaybackState::Paused)
                        {
                            // In DBus mode, call Resume() twice to force the sprite to update immediately.
#ifdef NO_DBUS
                            audioManager.Resume();
#else
                            audioManager.Resume();
                            audioManager.Resume();
#endif
                        }
                        else
                        {
                            audioManager.Play();
                        }
                        break;
                    case SDLK_RIGHT:
                        audioManager.NextTrack();
                        break;
                    case SDLK_LEFT:
                        // In DBus mode, if the playback position is >5s, issue an extra call.
#ifdef NO_DBUS
                        audioManager.PreviousTrack();
#else
                        if (audioManager.GetCurrentPlaybackPosition() > 5.0f)
                        {
                            audioManager.PreviousTrack();
                        }
                        audioManager.PreviousTrack();
#endif
                        break;
                    case SDLK_UP:
                        audioManager.SetVolume(audioManager.GetVolume() + 8);
                        break;
                    case SDLK_DOWN:
                        audioManager.SetVolume(audioManager.GetVolume() - 8);
                        break;
                    default:
                        break;
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // Update audio (DBus signals will update metadata)
        audioManager.Update(io.DeltaTime);

        // Draw one fullscreen ImGui window
        ImGuiWindowFlags wf = ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(window_width), static_cast<float>(window_height)));
        ImGui::Begin("Car Head Unit", nullptr, wf);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ui.Render(draw_list, audioManager, sprite, scale_x, scale_y);
        ImGui::End();

        // Render ImGui frame
        ImGui::Render();
        glViewport(0, 0, window_width, window_height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    ui.Cleanup();
    audioManager.Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
