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
    float artistY           = 11.0f - 5.0f;    // shifted up: originally 11, now 6
    float trackY            = 13.0f - 5.0f;    // shifted up: originally 13, now 8
    
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f - 5.0f;    // shifted up: now 17
    float timeTextYOffset   = 1.0f;            // unchanged
    float progressBarThickness = 0.25f;       // so that at scale=16, thickness=4 px

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;           // unchanged (adjust if needed)
    float spriteBaseY       = 28.25f - 5.0f;     // shifted up: now 23.25
    float spriteXCorrection = 0.250f;          // unchanged

    // -----------------------
    // Sun Volume Indicator
    // -----------------------
    float sunX       = 60.0f;  
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    // For the sun, shift sunMinY up by 5.
    float sunMinY    = 24.0f - 5.0f;   // now 19.0f
    // We'll use the same sunMaxY for both indicators—but the moon will be mirrored.
    float sunMaxY    = 8.0f;    // chosen so that the indicator doesn't clip
    // Adjust the mask accordingly (shifted up by 5).
    float sunMaskTop = 22.0f - 5.0f;    // now 17.0f
    float sunMaskBottom = 30.0f - 5.0f; // now 25.0f

    // -----------------------
    // Artist & Track Text
    // -----------------------
    // These values define the explicit text regions (in virtual units).
    // The artist name will appear under the left edge of the progress bar.
    // The track name will be right-aligned (starting at the right edge and growing inward).
    float artistTextX    = 15.0f;    // same as progressBarStartX
    float artistTextY    = 23.0f - 5.0f;   // now 18.0f
    float artistTextWidth = 25.0f;   // half of the progress bar width

    float trackTextX     = 65.0f;    // right edge of progress bar
    float trackTextY     = 23.0f - 5.0f;   // now 18.0f
    float trackTextWidth  = 25.0f;   // region width for track text (grows inward)
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

    // Sun Volume Indicator drawing.
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
