#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "BluetoothAudioManager.h"
#include "Sprite.h"
#include "Utilities.h"

// Layout configuration structure (virtual coordinates in an 80×30 space)
struct LayoutConfig {
    // -----------------------
    // Track Info + Progress Bar
    // -----------------------
    float trackRegionLeftX  = 15.0f;
    float trackRegionRightX = 65.0f;
    float artistY           = 11.0f - 5.0f;    // now 6.0
    float trackY            = 13.0f - 5.0f;    // now 8.0
    
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f - 5.0f;    // now 17.0
    float timeTextYOffset   = 1.0f;
    float progressBarThickness = 0.25f;        // stays the same

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;
    float spriteBaseY       = 28.25f - 5.0f;     // now 23.25
    float spriteXCorrection = 0.250f;

    // -----------------------
    // Sun/Moon Volume Indicator
    // -----------------------
    float sunX       = 60.0f;  
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    float sunMinY    = 24.0f - 5.0f;   // now 19.0
    float sunMaxY    = 8.0f;    // sun's top position
    // New field for moon: at full moon (vol=0), the moon's center will be at moonMaxY.
    float moonMaxY   = 17.0f;   // adjust as desired (e.g., 17.0 virtual units)
    float sunMaskTop = 22.0f - 5.0f;    // now 17.0
    float sunMaskBottom = 30.0f - 5.0f; // now 25.0

    // -----------------------
    // Artist & Track Text
    // -----------------------
    float artistTextX    = 15.0f;         // same as progressBarStartX.
    float artistTextY    = 23.0f - 5.0f;    // now 18.0.
    float artistTextWidth = 25.0f;         // half of the progress bar width.

    float trackTextX     = 65.0f - 25.0f;   // right-aligned: 40.0.
    float trackTextY     = 23.0f - 5.0f;      // now 18.0.
    float trackTextWidth  = 25.0f;          // half of the progress bar width.
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    void Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y);
    void Cleanup();

    LayoutConfig& GetLayoutConfig() { return layout; }

private:
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

    void DrawVolumeSun(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale,
                       float offset_x,
                       float offset_y);
    void DrawSunMask(ImDrawList* draw_list,
                     float scale,
                     float offset_x,
                     float offset_y);

    LayoutConfig layout;
};

#endif // UI_H
