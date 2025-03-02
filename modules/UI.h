#ifndef UI_H
#define UI_H

#include "imgui.h"
#include "BluetoothAudioManager.h"
#include "Sprite.h"
#include "Utilities.h"

// Layout configuration structure (virtual coordinates in an 80×30 space)
struct LayoutConfig {
    // Volume Bar
    float volumeLabelX      = 32.5f;
    float volumeLabelY      = 5.0f;
    // Gap between label and volume bar (in virtual units)
    float volumeBarTextGap  = 0.125f;  // ~2 px at baseline (scale=16)
    float volumeBarWidth    = 15.0f;
    float volumeBarHeight   = 1.0f;
    // Parameters for segmented cells
    int   volumeCellCount   = 10;      // number of segments
    float volumeCellGap     = 0.125f;  // gap between cells (in virtual units)
    
    // Track Info Region
    float trackRegionLeftX  = 15.0f;
    float trackRegionRightX = 65.0f;
    float artistY           = 11.0f;
    float trackY            = 13.0f;
    
    // Progress Bar (horizon)
    float progressBarStartX = 15.0f;
    float progressBarEndX   = 65.0f;
    float progressBarY      = 22.0f;
    float timeTextYOffset   = 1.0f;
    // Progress bar thickness (in virtual units) so that at baseline (e.g. scale=16) it equals 4px.
    float progressBarThickness = 0.25f;
    
    // Sprite Offsets (for the car)
    float spriteXOffset = 0.0f;
    float spriteYOffset = -8.0f;
    // Base Y position for sprite (in virtual units)
    float spriteBaseY   = 28.0f;
    // Correction for sprite’s horizontal travel (in virtual units).
    // Previously ~0.375f (~6px at scale=16) then adjusted; now set to 0.250f.
    float spriteXCorrection = 0.250f;
    
    // Sun (volume indicator) properties:
    float sunDiameter = 3.0f;           // in virtual units (≈48 px at scale=16)
    // Offset from the right end of the progress bar (in virtual units); sun center = progressBarEndX - sunHorizontalOffset.
    float sunHorizontalOffset = 0.75f;  
    // Sun mask: covers area below the progress bar to hide the sun when volume is low.
    float sunMaskHeight = 2.0f;         // in virtual units
    // Mask Bars (for covering the car sprite edges)
    float maskBarWidth  = 5.0f;
    float maskBarHeight = 3.0f;
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
    
    // New functions for the volume sun indicator.
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
