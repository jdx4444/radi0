#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "BluetoothAudioManager.h"
#include "Sprite.h"
#include "Utilities.h"

// Layout configuration structure (virtual coordinates in an 80×30 space)
struct LayoutConfig {
    // Volume Bar
    float volumeLabelX    = 32.5f;
    float volumeLabelY    = 5.0f;
    float volumeBarOffset = 2.0f;
    float volumeBarWidth  = 15.0f;
    float volumeBarHeight = 1.0f;
    // Track Info Region
    float trackRegionLeftX  = 15.0f;
    float trackRegionRightX = 65.0f;
    float artistY           = 11.0f;
    float trackY            = 13.0f;
    // Progress Bar
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f;
    float timeTextYOffset   = 1.0f;
    // New: progress bar thickness (in virtual units) 
    // (e.g., 0.25 * scale gives 4px when scale is 16)
    float progressBarThickness = 0.25f;
    // Sprite Offsets
    float spriteXOffset = -1.0f;
    float spriteYOffset = -8.0f;
    // Base Y position for sprite (in virtual units)
    float spriteBaseY   = 28.0f;
    // Mask Bars
    float maskBarWidth  = 5.0f;
    float maskBarHeight = 3.0f;
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    // Updated Render signature using unified scale and offsets
    void Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y);
    void Cleanup();

    // Expose layout config for adjustments
    LayoutConfig& GetLayoutConfig() { return layout; }

private:
    void DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale,
                       float offset_x,
                       float offset_y);
    void DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale,
                                float offset_x,
                                float offset_y);
    void DrawProgressLine(ImDrawList* draw_list,
                          BluetoothAudioManager& audioManager,
                          float scale,
                          float offset_x,
                          float offset_y);
    void DrawTimeRemaining(ImDrawList* draw_list,
                           BluetoothAudioManager& audioManager,
                           float scale,
                           float offset_x,
                           float offset_y);
    void DrawMaskBars(ImDrawList* draw_list,
                      float scale,
                      float offset_x,
                      float offset_y);

    LayoutConfig layout;
};

#endif // UI_H
