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
    float artistY           = 11.0f - 5.0f;    // originally 11, now 6.0
    float trackY            = 13.0f - 5.0f;    // originally 13, now 8.0
    
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f - 5.0f;    // originally 22, now 17.0
    float timeTextYOffset   = 1.0f;
    float progressBarThickness = 0.25f;        // so that at scale=16, thickness=4 px

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;           // unchanged
    float spriteBaseY       = 28.25f - 5.0f;     // originally 28.25, now 23.25
    float spriteXCorrection = 0.250f;          // unchanged

    // -----------------------
    // Sun/Moon Volume Indicator
    // -----------------------
    float sunX       = 60.0f;  
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    // For the indicator, shift sunMinY upward by 5.
    float sunMinY    = 24.0f - 5.0f;   // originally 24, now 19.0
    // sunMaxY is not used in the new mapping, but is kept here.
    float sunMaxY    = 8.0f;
    // The mask remains.
    float sunMaskTop = 22.0f - 5.0f;    // now 17.0
    float sunMaskBottom = 30.0f - 5.0f; // now 25.0

    // -----------------------
    // Artist & Track Text
    // -----------------------
    float artistTextX    = 15.0f;         // same as progressBarStartX.
    float artistTextY    = 23.0f - 5.0f;    // originally 23, now 18.0.
    float artistTextWidth = 25.0f;         // half of the progress bar width.

    // For right-aligned track text, set its region's left edge to (progressBarEndX - trackTextWidth).
    float trackTextX     = 65.0f - 25.0f;   // i.e. 40.0f.
    float trackTextY     = 23.0f - 5.0f;      // now 18.0f.
    float trackTextWidth  = 25.0f;          // half of the progress bar width.
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    // Render using unified scale and offsets.
    void Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y);
    void Cleanup();

    // Expose layout config for adjustments.
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

    // Sun/Moon Volume Indicator drawing.
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
