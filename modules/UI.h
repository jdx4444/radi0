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
    float artistY           = 11.0f;
    float trackY            = 13.0f;
    
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f;
    float timeTextYOffset   = 1.0f;
    float progressBarThickness = 0.25f;  // so that at scale=16, thickness=4 px

    // -----------------------
    // Car Sprite
    // -----------------------
    float spriteXOffset     = 0.0f;
    float spriteYOffset     = -8.0f;
    float spriteBaseY       = 27.0f;
    float spriteXCorrection = 0.250f; // Shifts the car’s travel horizontally

    // -----------------------
    // Sun Volume Indicator
    // -----------------------
    // Where the sun should be horizontally (in virtual units).
    // For example, near x=60 places it in the top-right quadrant of an 80×30 UI.
    float sunX       = 60.0f;  
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    // At volume=0, sun is at sunMinY (fully hidden below horizon).
    // At volume=128, sun is at sunMaxY (fully above horizon).
    float sunMinY    = 24.0f;   // just below horizon at y=22
    float sunMaxY    = 5.0f;    // near the top

    // Mask to hide the sun below the horizon.
    // If the horizon is at y=22, we can mask from y=22 downward.
    float sunMaskTop = 22.0f;
    float sunMaskBottom = 30.0f; // bottom of the screen is ~30 in virtual coords
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
    // --- [Removed the old DrawVolumeBar entirely] ---

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

    // Sun Volume Indicator
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
