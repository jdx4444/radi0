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
    float artistY           = 11.0f - 5.0f;  // shifted up to 6.0f
    float trackY            = 13.0f - 5.0f;  // shifted up to 8.0f
    
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f - 5.0f;  // now 17.0f
    float timeTextYOffset   = 1.0f;          // unchanged
    float progressBarThickness = 0.25f;      // stays the same

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;         // unchanged (if you wish, you can also shift this)
    float spriteBaseY       = 28.25f - 5.0f;   // now 23.25f
    float spriteXCorrection = 0.250f;        // unchanged

    // -----------------------
    // Sun Volume Indicator
    // -----------------------
    float sunX       = 60.0f;  
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    // Shift sunMinY up by 5 units:
    float sunMinY    = 24.0f - 5.0f;   // now 19.0f
    // Override max height to 20.0f:
    float sunMaxY    = 20.0f;
    // Also shift the mask values up by 5.
    float sunMaskTop = 22.0f - 5.0f;    // now 17.0f
    float sunMaskBottom = 30.0f - 5.0f; // now 25.0f

    // -----------------------
    // Artist & Track Text
    // -----------------------
    // Define explicit text regions.
    float artistTextX    = 15.0f;         // same as progressBarStartX
    float artistTextY    = 23.0f - 5.0f;    // now 18.0f (just below progress bar)
    float artistTextWidth = 25.0f;          // half of progress bar width

    float trackTextX     = 40.0f;          // halfway along progress bar
    float trackTextY     = 23.0f - 5.0f;    // now 18.0f
    float trackTextWidth  = 25.0f;          // half of progress bar width
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
