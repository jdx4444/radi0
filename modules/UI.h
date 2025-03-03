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
    float timeTextYOffset   = 1.0f;            // (no longer used)
    float progressBarThickness = 0.25f;        // so that at scale=16, thickness=4 px

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;           // unchanged
    float spriteBaseY       = 28.25f - 5.0f;     // originally 28.25, now 23.25
    float spriteXCorrection = 0.250f;          // unchanged

    // -----------------------
    // Sun/Moon Volume Indicator (Circular Path)
    // -----------------------
    // We will compute the indicator's position along a circle whose horizontal center is the midpoint
    // of the progress bar and whose vertical span is fixed.
    // For our example, we define:
    //   center_x = (progressBarStartX + progressBarEndX) / 2 = 40.
    //   We want the top of the circle to be at 8 and the bottom at 17.
    //   So radius R = (17 - 8)/2 = 4.5 and center_y = (8 + 17)/2 = 12.5.
    float sunX       = 40.0f;  // (will use the progress bar center)
    float indicatorCenterY = 12.5f; // computed center Y for the circle
    float indicatorRadius  = 4.5f;  // radius of the circle

    // We still keep the sunDiameter (for drawing the indicator's body) unchanged.
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    // The horizon (progress bar) is at progressBarY (17 virtual units).
    // The mask remains as before.
    float sunMaskTop = 22.0f - 5.0f;    // now 17.0
    float sunMaskBottom = 30.0f - 5.0f; // now 25.0

    // -----------------------
    // Artist & Track Text
    // -----------------------
    float artistTextX    = 15.0f;         // same as progressBarStartX.
    float artistTextY    = 23.0f - 5.0f;    // originally 23, now 18.0.
    float artistTextWidth = 25.0f;         // half of progress bar width (50/2)

    // Track text: right-aligned; its region starts at (progressBarEndX - trackTextWidth).
    float trackTextX     = 65.0f - 25.0f;   // i.e. 40.0f.
    float trackTextY     = 23.0f - 5.0f;      // now 18.0f.
    float trackTextWidth  = 25.0f;          // half of progress bar width.
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

    // We remove DrawTimeRemaining so that the track progress number is gone.

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
