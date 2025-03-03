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
    float spriteBaseY       = 28.25f;
    float spriteXCorrection = 0.250f; // Shifts the car’s travel horizontally

    // -----------------------
    // Sun Volume Indicator
    // -----------------------
    float sunX       = 60.0f;  
    float sunDiameter = 3.0f;   // ~48 px at scale=16
    float sunMinY    = 24.0f;   // just below horizon at y=22
    float sunMaxY    = 5.0f;    // near the top
    float sunMaskTop = 22.0f;
    float sunMaskBottom = 30.0f; // bottom of the screen is ~30 in virtual coords

    // -----------------------
    // Artist & Track Text
    // -----------------------
    // These values define the explicit text regions (in virtual units)
    // where the artist and track names are drawn.
    // The artist name will appear under the left bottom edge of the progress bar.
    // The track name will begin at the halfway mark.
    float artistTextX    = 15.0f;   // e.g. same as progressBarStartX
    float artistTextY    = 23.0f;   // just below the progress bar (progressBarY=22)
    float artistTextWidth = 25.0f;  // half of the progress bar width (50/2)

    float trackTextX     = 40.0f;   // halfway along the progress bar (15 + 25)
    float trackTextY     = 23.0f;   // same vertical position as artist text
    float trackTextWidth  = 25.0f;  // half of the progress bar width
};

class UI {
public:
    UI();
    ~UI();

    void Initialize();
    // Updated Render signature using unified scale and offsets.
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
    // Artist & Track Text drawing.
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
