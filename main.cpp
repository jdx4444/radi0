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

const float VIRTUAL_WIDTH = 20.0f;
const float VIRTUAL_HEIGHT = 10.0f;
const float SCREEN_WIDTH_IN = 5.0f;
const float SCREEN_HEIGHT_IN = 2.5f;
const float SCREEN_DPI = 96.0f;

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0) {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

#if defined(IMGUI_IMPL_OPENGL_ES2)
    const char* glsl_version = "#version 100";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
#elif defined(__APPLE__)
    const char* glsl_version = "#version 150";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,2);
#else
    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);
#endif

#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    int window_width = (int)(SCREEN_WIDTH_IN * SCREEN_DPI);
    int window_height = (int)(SCREEN_HEIGHT_IN * SCREEN_DPI);

    float scale_x = window_width / VIRTUAL_WIDTH;
    float scale_y = window_height / VIRTUAL_HEIGHT;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE,8);

    SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_BORDERLESS);
    SDL_Window* window = SDL_CreateWindow("Retro Car Head Unit", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, window_width, window_height, window_flags);
    if (!window) {
        printf("Error: SDL_CreateWindow(): %s\n", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    SDL_SetWindowResizable(window, SDL_FALSE);
    SDL_SetWindowPosition(window, 0, 0);

    SDL_GLContext gl_context = SDL_GL_CreateContext(window);
    if (!gl_context) {
        printf("Error: SDL_GL_CreateContext(): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    SDL_GL_MakeCurrent(window, gl_context);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();(void)io;
    io.Fonts->AddFontDefault();

    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowPadding = ImVec2(0,0);

    ImVec4* colors = style.Colors;
    colors[ImGuiCol_WindowBg]      = ImVec4(0,0,0,1);
    colors[ImGuiCol_Text]          = ImVec4(0,1,0,1);
    colors[ImGuiCol_Border]        = ImVec4(0,1,0,1);
    colors[ImGuiCol_FrameBg]       = ImVec4(0,0,0,1);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0,1,0,1);

    ImVec4 clear_color = ImVec4(0,0,0,1);

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Initialize BluetoothAudioManager in place of AudioManager
    BluetoothAudioManager audioManager;
    if (!audioManager.Initialize()) {
        printf("Failed to initialize BluetoothAudioManager.\n");
        return -1;
    }
    audioManager.AddToPlaylist("Track1.mp3", 180.0f);
    audioManager.AddToPlaylist("Track2.mp3", 200.0f);
    audioManager.Play();

    // Initialize Sprite
    Sprite sprite;
    sprite.Initialize(scale_x, scale_y);

    UI ui;
    ui.Initialize();

    bool done = false;

    while (!done) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT)
                done = true;
            if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE)
                done = true;

            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_SPACE:
                        if (audioManager.GetState() == PlaybackState::Playing)
                            audioManager.Pause();
                        else if (audioManager.GetState() == PlaybackState::Paused)
                            audioManager.Resume();
                        else
                            audioManager.Play();
                        break;
                    case SDLK_RIGHT:
                        audioManager.NextTrack();
                        break;
                    case SDLK_LEFT:
                        audioManager.PreviousTrack();
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

        audioManager.Update(io.DeltaTime);

        ImGuiWindowFlags wf = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse;

        ImGui::SetNextWindowPos(ImVec2(0,0));
        ImGui::SetNextWindowSize(ImVec2((float)window_width,(float)window_height));

        ImGui::Begin("Car Head Unit", nullptr, wf);

        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ui.Render(draw_list, audioManager, sprite, scale_x, scale_y);

        ImGui::End();

        ImGui::Render();
        glViewport(0,0,window_width,window_height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

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
