#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "IAudioManager.h"   // Use the common interface
#include "Sprite.h"
#include "Utilities.h"

// Layout configuration structure (virtual coordinates in an 80×25 space)
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
    float progressBarThickness = 0.25f;        // so that at scale=16, thickness=4 px

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;           // unchanged
    float spriteBaseY       = 28.25f - 5.0f;     // originally 28.25, now 23.25
    float spriteXCorrection = -0.150f;          // unchanged

    // -----------------------
    // Volume Indicator (Sun/Moon) – Circular Path
    // -----------------------
    float indicatorCenterX = 40.0f;
    float indicatorCenterY = 8.5f;  
    float indicatorRadius  = 10.0f;  

    float sunDiameter = 3.0f;   // ~48 px at scale=16
    float sunMaskTop = 22.0f - 5.0f;    // now 17.0
    float sunMaskBottom = 30.0f - 5.0f; // now 25.0

    // -----------------------
    // Artist & Track Text
    // -----------------------
    float artistTextX    = 15.0f;
    float artistTextY    = 23.0f - 5.0f;   // now 18.0.
    float artistTextWidth = 25.0f;

    float trackTextX     = 65.0f - 25.0f;  // i.e. 40.0f.
    float trackTextY     = 23.0f - 5.0f;     // now 18.0f.
    float trackTextWidth  = 25.0f;

    // -----------------------
    // New: Border Padding for UI
    // -----------------------
    float borderPadding = 2.0f;  // Padding in virtual units.
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    // Changed Render signature: audioManager is now an IAudioManager&
    void Render(ImDrawList* draw_list,
                IAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y,
                int window_width,
                int window_height);
    void Cleanup();

    LayoutConfig& GetLayoutConfig() { return layout; }

private:
    void DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                IAudioManager& audioManager,
                                float scale,
                                float offset_x,
                                float offset_y);

    void DrawProgressLine(ImDrawList* draw_list,
                          IAudioManager& audioManager,
                          float scale,
                          float offset_x,
                          float offset_y);

    void DrawMaskBars(ImDrawList* draw_list,
                      float scale,
                      float offset_x,
                      float offset_y);

    // Volume Indicator drawing.
    void DrawVolumeSun(ImDrawList* draw_list,
                       IAudioManager& audioManager,
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
