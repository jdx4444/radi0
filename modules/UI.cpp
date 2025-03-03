#include "UI.h"
#include <algorithm>
#include <string>
#include <cmath>

// Define some colors (using the new hex #6dfe95)
const ImU32 COLOR_GREEN = IM_COL32(109, 254, 149, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

// We'll use a verticalScale factor to reduce the vertical travel on the circle.
static const float verticalScale = 1.0f; // For a full circle, set to 1.0. (Adjust if needed)
  
UI::UI() {}
UI::~UI() {}

void UI::Initialize()
{
    // No special initialization needed.
}

// Render order: Draw the volume indicator first (so it appears behind the progress line).
void UI::Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y)
{
    // 1) Draw the volume indicator (sun/moon) behind the progress line.
    DrawVolumeSun(draw_list, audioManager, scale, offset_x, offset_y);
    
    // 2) Draw the progress line and time remaining on top.
    DrawProgressLine(draw_list, audioManager, scale, offset_x, offset_y);
    DrawTimeRemaining(draw_list, audioManager, scale, offset_x, offset_y);
    
    // 3) Draw the car sprite and its mask bars.
    constexpr float spriteVirtualWidth = 19 * 0.16f; // ~3.04 virtual units.
    float effectiveStartX = layout.progressBarStartX - spriteVirtualWidth + layout.spriteXCorrection;
    float effectiveEndX   = layout.progressBarEndX + layout.spriteXCorrection;
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(),
                          effectiveStartX, effectiveEndX,
                          scale, offset_x, offset_y,
                          layout.spriteXOffset,
                          layout.spriteYOffset,
                          layout.spriteBaseY);
    sprite.Draw(draw_list, COLOR_GREEN);
    DrawMaskBars(draw_list, scale, offset_x, offset_y);
    
    // 4) Draw the artist and track info on top.
    DrawArtistAndTrackInfo(draw_list, audioManager, scale, offset_x, offset_y);
}

void UI::Cleanup()
{
    // No cleanup needed.
}

/**
 * DrawArtistAndTrackInfo:
 * Uses explicit layout values from LayoutConfig.
 * The artist text is drawn in the region starting at (artistTextX, artistTextY)
 * with a maximum width of artistTextWidth.
 * The track text is drawn in the region starting at (trackTextX, trackTextY)
 * with a maximum width of trackTextWidth, right-aligned.
 * If the text exceeds the available width, it scrolls.
 */
void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale,
                                float offset_x,
                                float offset_y)
{
    std::string artist_name = audioManager.GetCurrentTrackArtist();
    std::string track_name  = audioManager.GetCurrentTrackTitle();
    if (artist_name.empty()) artist_name = "Unknown Artist";
    if (track_name.empty()) track_name  = "Unknown Track";

    // Artist text region.
    ImVec2 artistPos = ToPixels(layout.artistTextX, layout.artistTextY, scale, offset_x, offset_y);
    float artistRegionWidth_px = layout.artistTextWidth * scale;
    ImGui::SetWindowFontScale(1.6f);
    ImVec2 artistTextSize = ImGui::CalcTextSize(artist_name.c_str());
    static float artist_scroll_offset = 0.0f;
    float dt = ImGui::GetIO().DeltaTime * 30.0f;
    if (artistTextSize.x > artistRegionWidth_px) {
        artist_scroll_offset += dt;
        if (artist_scroll_offset > artistTextSize.x)
            artist_scroll_offset = -artistRegionWidth_px;
    } else {
        artist_scroll_offset = 0.0f;
    }
    ImVec2 artistClipMin = artistPos;
    ImVec2 artistClipMax = ImVec2(artistPos.x + artistRegionWidth_px, artistPos.y + artistTextSize.y);
    draw_list->PushClipRect(artistClipMin, artistClipMax, true);
    ImGui::SetCursorPos(ImVec2(artistPos.x - artist_scroll_offset, artistPos.y));
    ImGui::TextUnformatted(artist_name.c_str());
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);

    // Track text region (right-aligned).
    ImVec2 trackPos = ToPixels(layout.trackTextX, layout.trackTextY, scale, offset_x, offset_y);
    float trackRegionWidth_px = layout.trackTextWidth * scale;
    ImGui::SetWindowFontScale(1.6f);
    ImVec2 trackTextSize = ImGui::CalcTextSize(track_name.c_str());
    static float track_scroll_offset = 0.0f;
    if (trackTextSize.x > trackRegionWidth_px) {
        track_scroll_offset += dt;
        if (track_scroll_offset > trackTextSize.x)
            track_scroll_offset = -trackRegionWidth_px;
    } else {
        track_scroll_offset = 0.0f;
    }
    ImVec2 trackClipMin = trackPos;
    ImVec2 trackClipMax = ImVec2(trackPos.x + trackRegionWidth_px, trackPos.y + trackTextSize.y);
    draw_list->PushClipRect(trackClipMin, trackClipMax, true);
    float textOffset = trackTextSize.x - trackRegionWidth_px;
    if (textOffset < 0) textOffset = 0;
    ImGui::SetCursorPos(ImVec2(trackPos.x - track_scroll_offset - textOffset, trackPos.y));
    ImGui::TextUnformatted(track_name.c_str());
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);
}

void UI::DrawProgressLine(ImDrawList* draw_list,
                          BluetoothAudioManager& audioManager,
                          float scale,
                          float offset_x,
                          float offset_y)
{
    ImVec2 p1 = ToPixels(layout.progressBarStartX, layout.progressBarY, scale, offset_x, offset_y);
    ImVec2 p2 = ToPixels(layout.progressBarEndX, layout.progressBarY, scale, offset_x, offset_y);
    float line_thickness = layout.progressBarThickness * scale;
    draw_list->AddLine(p1, p2, COLOR_GREEN, line_thickness);
}

void UI::DrawTimeRemaining(ImDrawList* draw_list,
                           BluetoothAudioManager& audioManager,
                           float scale,
                           float offset_x,
                           float offset_y)
{
    float region_width = layout.progressBarEndX - layout.progressBarStartX;
    ImVec2 region_pos = ToPixels(layout.progressBarStartX,
                                 layout.progressBarY + layout.timeTextYOffset,
                                 scale, offset_x, offset_y);
    std::string time_str = audioManager.GetTimeRemaining();
    ImVec2 text_size = ImGui::CalcTextSize(time_str.c_str());
    float region_width_pixels = region_width * scale;
    float text_x = region_pos.x + (region_width_pixels - text_size.x) * 0.5f;
    float text_y = region_pos.y;
    ImGui::SetCursorPos(ImVec2(text_x, text_y));
    ImGui::Text("%s", time_str.c_str());
}

void UI::DrawMaskBars(ImDrawList* draw_list,
                      float scale,
                      float offset_x,
                      float offset_y)
{
    float mask_width  = 5.0f;
    float mask_height = 3.0f;
    ImVec2 left_top  = ToPixels(layout.progressBarStartX - mask_width,
                                layout.progressBarY - mask_height,
                                scale, offset_x, offset_y);
    ImVec2 left_bot  = ToPixels(layout.progressBarStartX,
                                layout.progressBarY + mask_height,
                                scale, offset_x, offset_y);
    ImVec2 right_top = ToPixels(layout.progressBarEndX,
                                layout.progressBarY - mask_height,
                                scale, offset_x, offset_y);
    ImVec2 right_bot = ToPixels(layout.progressBarEndX + mask_width,
                                layout.progressBarY + mask_height,
                                scale, offset_x, offset_y);
    draw_list->AddRectFilled(left_top, left_bot, COLOR_BLACK);
    draw_list->AddRectFilled(right_top, right_bot, COLOR_BLACK);
}

/**
 * DrawVolumeSun:
 * We define a circular path for the indicator.
 * Let horizon = progressBarY, and choose R (the radius of the circular path) and delta (a small offset)
 * so that the circle’s bottom (C_y + R) is just below the horizon.
 *
 * For vol in [64,128] (the sun branch):
 *    t = (vol - 64)/64, then θ = π/2 – (π/2)*t.
 *    x = Cₓ + R*cos(θ), y = C_y + R*sin(θ).
 *
 * For vol in [0,64] (the moon branch):
 *    t = (64 - vol)/64, then θ = π/2 – (π/2)*t.
 *    x = Cₓ - R*cos(θ) (mirrored horizontally about center at 40), y = C_y + R*sin(θ).
 */
void UI::DrawVolumeSun(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale,
                       float offset_x,
                       float offset_y)
{
    // Define our circle parameters.
    float horizon = layout.progressBarY; // virtual units (e.g. 17.0)
    float R = 3.0f;                      // radius in virtual units.
    float delta = 0.2f;                  // indicator bottom is horizon + delta.
    float C_x = layout.sunX;             // use layout.sunX.
    float C_y = horizon + delta - R;       // so that C_y + R = horizon + delta.
    
    float vol = static_cast<float>(audioManager.GetVolume());
    if (vol >= 64.0f) {
        // Sun branch.
        float t = (vol - 64.0f) / 64.0f;  // t in [0,1]
        float theta = (3.1415926f / 2.0f) - ((3.1415926f / 2.0f) * t);  // from 90° to 0°
        float x = C_x + R * std::cos(theta);
        float y = C_y + R * std::sin(theta);
        ImVec2 sun_center = ToPixels(x, y, scale, offset_x, offset_y);
        float bodyScale = 0.25f;  // as provided.
        float sun_radius_px = (layout.sunDiameter * bodyScale * scale) * 0.5f;
        draw_list->AddCircleFilled(sun_center, sun_radius_px, COLOR_GREEN, 32);
        int numRays = 8;
        float gap = 4.0f;
        float rayLength = sun_radius_px * 1.0f;
        float rayLineWidth = 1.0f;
        for (int i = 0; i < numRays; i++) {
            float angle = (3.1415926f * 2.0f / numRays) * i;
            ImVec2 rayStart(
                sun_center.x + std::cos(angle) * (sun_radius_px + gap),
                sun_center.y + std::sin(angle) * (sun_radius_px + gap)
            );
            ImVec2 rayEnd(
                sun_center.x + std::cos(angle) * (sun_radius_px + gap + rayLength),
                sun_center.y + std::sin(angle) * (sun_radius_px + gap + rayLength)
            );
            draw_list->AddLine(rayStart, rayEnd, COLOR_GREEN, rayLineWidth);
        }
    } else {
        // Moon branch.
        float t = (64.0f - vol) / 64.0f;  // t in [0,1]
        float theta = (3.1415926f / 2.0f) - ((3.1415926f / 2.0f) * t);
        float x = C_x - R * std::cos(theta);  // mirror horizontally
        float y = C_y + R * std::sin(theta);
        // Mirror x about virtual center 40.
        float x_moon = 2 * 40.0f - x;
        ImVec2 moon_center = ToPixels(x_moon, y, scale, offset_x, offset_y);
        float bodyScale = 0.75f; // Moon is 3x larger.
        float moon_radius_px = (layout.sunDiameter * bodyScale * scale) * 0.5f;
        draw_list->AddCircleFilled(moon_center, moon_radius_px, COLOR_GREEN, 32);
        // Draw a crescent by overlaying an offset black circle.
        ImVec2 offsetVec = ImVec2(moon_radius_px * 0.4f, -moon_radius_px * 0.2f);
        ImVec2 cutoutCenter = ImVec2(moon_center.x + offsetVec.x, moon_center.y + offsetVec.y);
        draw_list->AddCircleFilled(cutoutCenter, moon_radius_px, COLOR_BLACK, 32);
    }
    
    DrawSunMask(draw_list, scale, offset_x, offset_y);
}

void UI::DrawSunMask(ImDrawList* draw_list,
                     float scale,
                     float offset_x,
                     float offset_y)
{
    float left   = 0.0f;
    float right  = 80.0f;
    float top    = layout.sunMaskTop;
    float bottom = layout.sunMaskBottom;
    ImVec2 p1 = ToPixels(left, top, scale, offset_x, offset_y);
    ImVec2 p2 = ToPixels(right, bottom, scale, offset_x, offset_y);
    draw_list->AddRectFilled(p1, p2, COLOR_BLACK);
}
