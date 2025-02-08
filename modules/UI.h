#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "BluetoothAudioManager.h"
#include "Sprite.h"
#include "Utilities.h"

//------------------------------------------------------------------------------
// LayoutConfig: A single place to store all virtual-coordinate layout values.
// Adjust these values to reposition elements in your 80×30 virtual space.
//------------------------------------------------------------------------------
struct LayoutConfig {
    // --- Volume Bar ---
    float volumeLabelX    = 25.0f;
    float volumeLabelY    = 5.0f;
    float volumeBarOffset = 7.0f;   // How far right from the label the bar starts
    float volumeBarWidth  = 15.0f;  // Virtual width of the volume bar
    float volumeBarHeight = 1.0f;   // Virtual height of the volume bar

    // --- Track Info Region ---
    float trackRegionLeftX  = 15.0f;
    float trackRegionRightX = 65.0f;
    float artistY           = 11.0f;
    float trackY            = 13.0f;

    // --- Progress Bar ---
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f;
    float timeTextYOffset   = 1.0f; // Vertical offset below the progress bar

    // --- Sprite Offsets ---
    float spriteXOffset = -1.0f;
    float spriteYOffset = -8.0f;
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    void Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale_x, float scale_y);
    void Cleanup();

    // Expose layout config so it can be adjusted (if needed) from elsewhere
    LayoutConfig& GetLayoutConfig() { return layout; }

private:
    // Helper functions that now rely on layout configuration values
    void DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale_x, float scale_y);
    void DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale_x, float scale_y);
    void DrawProgressLine(ImDrawList* draw_list,
                          BluetoothAudioManager& audioManager,
                          float scale_x, float scale_y);
    void DrawTimeRemaining(ImDrawList* draw_list,
                           BluetoothAudioManager& audioManager,
                           float scale_x, float scale_y);
    void DrawMaskBars(ImDrawList* draw_list,
                      float scale_x, float scale_y);

    LayoutConfig layout;
};

#endif // UI_H
