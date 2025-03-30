#include <stdio.h>
#include <SDL.h>
#include <sys/stat.h>
#include <memory>
#include <atomic>
#ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_opengl3.h"

#include "IAudioManager.h"
#include "BluetoothAudioManager.h"
#include "USBAudioManager.h"
#include "modules/Sprite.h"
#include "modules/UI.h"
#include "ExhaustEffect.h"    // NEW: Include our exhaust effect header
#include <algorithm>

// Utility function to check if a directory exists.
bool directoryExists(const char *path) {
    struct stat info;
    return (stat(path, &info) == 0 && (info.st_mode & S_IFDIR));
}

// Enum to track current audio mode.
enum AudioMode { USB_MODE, BLUETOOTH_MODE };
static AudioMode currentAudioMode;

// Define a virtual coordinate system (80x25).
static const float VIRTUAL_WIDTH = 80.0f;
static const float VIRTUAL_HEIGHT = 25.0f;

// Global atomic flag to prevent overlapping mode switches.
std::atomic<bool> switchInProgress(false);

int main(int, char**)
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("Error: %s\n", SDL_GetError());
        return -1;
    }

    const char* glsl_version = "#version 130";
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#ifdef SDL_HINT_IME_SHOW_UI
    SDL_SetHint(SDL_HINT_IME_SHOW_UI, "1");
#endif

    SDL_DisplayMode DM;
    if (SDL_GetCurrentDisplayMode(0, &DM) != 0)
    {
        printf("SDL_GetCurrentDisplayMode failed: %s\n", SDL_GetError());
        return -1;
    }
    int window_width = DM.w;
    int window_height = DM.h;

    float scale = std::min(static_cast<float>(window_width) / VIRTUAL_WIDTH,
                           static_cast<float>(window_height) / VIRTUAL_HEIGHT);
    float offset_x = (window_width - VIRTUAL_WIDTH * scale) * 0.5f;
    float offset_y = (window_height - VIRTUAL_HEIGHT * scale) * 0.5f;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

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
    SDL_SetRelativeMouseMode(SDL_TRUE);
    printf("Cursor state after disabling: %d\n", SDL_ShowCursor(-1));
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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->AddFontDefault();
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.WindowPadding = ImVec2(0, 0);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0, 0, 0, 1);
    ImVec4 newColor = ImVec4(109/255.f, 254/255.f, 149/255.f, 1.0f);
    style.Colors[ImGuiCol_Text] = newColor;
    style.Colors[ImGuiCol_Border] = newColor;
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0, 0, 0, 1);
    style.Colors[ImGuiCol_PlotHistogram] = newColor;
    ImVec4 clear_color = ImVec4(0, 0, 0, 1);

    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Determine initial audio mode.
    if (directoryExists("/media/jdx4444/Mustick"))
         currentAudioMode = USB_MODE;
    else
         currentAudioMode = BLUETOOTH_MODE;

    std::unique_ptr<IAudioManager> audioManager;
    if (currentAudioMode == USB_MODE) {
         audioManager = std::make_unique<USBAudioManager>();
         printf("Using USB Audio Manager.\n");
    } else {
         audioManager = std::make_unique<BluetoothAudioManager>();
         printf("Using Bluetooth Audio Manager.\n");
    }
    if (!audioManager->Initialize())
    {
        printf("Failed to initialize audio manager.\n");
        return -1;
    }
    audioManager->SetVolume(20); // Set default volume low
    audioManager->Play();

    Sprite sprite;
    sprite.Initialize(scale);
    UI ui;
    ui.Initialize();

    // Create our exhaust effect instance.
    ExhaustEffect exhaustEffect;

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
            if (event.type == SDL_KEYDOWN)
            {
                SDL_Keycode key = event.key.keysym.sym;
                if (switchInProgress.load()) {
                    continue;
                }
                switch (key) {
                    case SDLK_x:
                        done = true;
                        break;
                        case SDLK_e:
                            switchInProgress.store(true);
                            if (currentAudioMode == USB_MODE) {
                                auto tempBt = std::make_unique<BluetoothAudioManager>();
                                if (!tempBt->Initialize() || !tempBt->IsPaired()) {
                                    printf("No paired phone found. Remaining in USB mode.\n");
                                } else {
                                    audioManager->Shutdown();
                                    audioManager = std::move(tempBt);
                                    currentAudioMode = BLUETOOTH_MODE;
                                    printf("Switched to Bluetooth Audio Manager.\n");
                                    audioManager->SetVolume(20);  // Set default low volume
                                    audioManager->Play();
                                    // Force metadata refresh immediately.
                                    dynamic_cast<BluetoothAudioManager*>(audioManager.get())->ForceMetadataRefresh();
                                }
                            } else {
                                if (directoryExists("/media/jdx4444/Mustick")) {
                                    audioManager->Shutdown();
                                    SDL_Delay(1500);
                                    audioManager = std::make_unique<USBAudioManager>();
                                    currentAudioMode = USB_MODE;
                                    printf("Switched to USB Audio Manager.\n");
                                    if (!audioManager->Initialize()) {
                                        printf("Failed to reinitialize USB Audio Manager.\n");
                                    } else {
                                        audioManager->SetVolume(20);  // Set default low volume
                                        audioManager->Play();
                                    }
                                } else {
                                    printf("USB drive not available. Remaining in Bluetooth mode.\n");
                                }
                            }
                            switchInProgress.store(false);
                            break;
                    case SDLK_SPACE:
                        if (audioManager->GetState() == PlaybackState::Playing)
                            audioManager->Pause();
                        else if (audioManager->GetState() == PlaybackState::Paused) {
                            audioManager->Resume();
                            // When resuming, trigger exhaust puff using sprite’s exhaust position.
                            exhaustEffect.Trigger(sprite.GetExhaustPosition());
                        }
                        else
                            audioManager->Play();
                        break;
                    case SDLK_RIGHT:
                        audioManager->NextTrack();
                        break;
                    case SDLK_LEFT:
                        audioManager->PreviousTrack();
                        break;
                    case SDLK_UP:
                        audioManager->SetVolume(audioManager->GetVolume() + 8);
                        break;
                    case SDLK_DOWN:
                        audioManager->SetVolume(audioManager->GetVolume() - 8);
                        break;
                    default:
                        break;
                }
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        audioManager->Update(io.DeltaTime);

        // Draw main UI window without borders.
        ImGuiWindowFlags wf = ImGuiWindowFlags_NoResize |
                              ImGuiWindowFlags_NoMove |
                              ImGuiWindowFlags_NoTitleBar |
                              ImGuiWindowFlags_NoScrollbar |
                              ImGuiWindowFlags_NoScrollWithMouse;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f); // Remove default border
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(window_width), static_cast<float>(window_height)));
        ImGui::Begin("Car Head Unit", nullptr, wf);
        {
            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ui.Render(draw_list, *audioManager, sprite, scale, offset_x, offset_y, window_width, window_height);
        }
        ImGui::End();
        ImGui::PopStyleVar();

        // Draw overlay window for exhaust effect, mask bars, borders, and status box.
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(window_width), static_cast<float>(window_height)));
        ImGui::Begin("Exhaust & Mask Overlay", nullptr,
                    ImGuiWindowFlags_NoTitleBar |
                    ImGuiWindowFlags_NoResize |
                    ImGuiWindowFlags_NoMove |
                    ImGuiWindowFlags_NoScrollbar |
                    ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_NoBackground);
        {
            exhaustEffect.Update(io.DeltaTime);
            ImDrawList* overlay_draw_list = ImGui::GetWindowDrawList();
            exhaustEffect.Draw(overlay_draw_list);
            ui.DrawMaskBars(overlay_draw_list, scale, offset_x, offset_y);
            ui.DrawBorders(overlay_draw_list, window_width, window_height);
        }
        ImGui::End();

        ImGui::Render();
        glViewport(0, 0, window_width, window_height);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ui.Cleanup();
    audioManager->Shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
