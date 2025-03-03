#include "UI.h"
#include <algorithm>
#include <string>
#include <cmath>

// Define some colors (using the new hex #6dfe95)
const ImU32 COLOR_GREEN = IM_COL32(109, 254, 149, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

UI::UI() {}
UI::~UI() {}

void UI::Initialize()
{
    // No special initialization needed.
}

void UI::Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y)
{
    // 1) Draw the progress line (the "horizon") and time remaining.
    DrawProgressLine(draw_list, audioManager, scale, offset_x, offset_y);
    DrawTimeRemaining(draw_list, audioManager, scale, offset_x, offset_y);
    
    // 2) Draw the volume indicator.
    // For volume >= 64, draw the sun; for volume < 64, draw the moon.
    float vol = static_cast<float>(audioManager.GetVolume());
    if (vol >= 64.0f) {
        // Sun branch.
        float u = (vol - 64.0f) / 64.0f; // u in [0,1]
        float bx = layout.sunX - 10.0f * u * u;
        float by = layout.sunMinY - (layout.sunMinY - layout.sunMaxY) * u * u;
        ImVec2 sun_center = ToPixels(bx, by, scale, offset_x, offset_y);
        float bodyScale = 0.25f;  // as provided for sun.
        float sun_radius_px = (layout.sunDiameter * bodyScale * scale) * 0.5f;
        draw_list->AddCircleFilled(sun_center, sun_radius_px, COLOR_GREEN, 32);
        int numRays = 8;
        float gap = 4.0f;             // gap in pixels.
        float rayLength = sun_radius_px * 1.0f; // extend rays 1.0x beyond gap.
        float rayLineWidth = 1.0f;     // ray thickness.
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
        // Define v = (64 - vol)/64 so that at vol=64, v=0 and at vol=0, v=1.
        float v = (64.0f - vol) / 64.0f;
        // Use the same vertical mapping as the sun:
        float bx = layout.sunX - 10.0f * v * v;
        float by = layout.sunMinY - (layout.sunMinY - layout.sunMaxY) * v * v;
        // Mirror the horizontal coordinate about 40 virtual units.
        float bx_moon = 2 * 40.0f - bx;
        ImVec2 moon_center = ToPixels(bx_moon, by, scale, offset_x, offset_y);
        // Make the moon 3x larger.
        float bodyScale = 0.75f;
        float moon_radius_px = (layout.sunDiameter * bodyScale * scale) * 0.5f;
        draw_list->AddCircleFilled(moon_center, moon_radius_px, COLOR_GREEN, 32);
        // Create a crescent by drawing an offset black circle over the moon.
        ImVec2 offsetVec = ImVec2(moon_radius_px * 0.4f, -moon_radius_px * 0.2f);
        ImVec2 cutoutCenter = ImVec2(moon_center.x + offsetVec.x, moon_center.y + offsetVec.y);
        draw_list->AddCircleFilled(cutoutCenter, moon_radius_px, COLOR_BLACK, 32);
    }
    
    DrawSunMask(draw_list, scale, offset_x, offset_y);

    // 3) Draw the car sprite and its side mask bars.
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
    ImGui::SetWindowFontScale(1.6f);  // same as artist.
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
    // Right-align: shift so the text's right edge is flush with the region's right edge.
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

void UI::DrawVolumeSun(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale,
                       float offset_x,
                       float offset_y)
{
    float vol = static_cast<float>(audioManager.GetVolume());
    float tau = 0.125f; // threshold (about 2 of 16 increments)
    if (vol >= 64.0f) {
        // Sun branch.
        float u = (vol - 64.0f) / 64.0f;
        float bx = layout.sunX - 10.0f * u * u;
        float by = layout.sunMinY - (layout.sunMinY - layout.sunMaxY) * u * u;
        ImVec2 sun_center = ToPixels(bx, by, scale, offset_x, offset_y);
        float bodyScale = 0.25f;
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
        float v = (64.0f - vol) / 64.0f; // v in [0,1]
        float bx = layout.sunX - 10.0f * v * v;
        float by = layout.sunMinY - (layout.sunMinY - layout.sunMaxY) * v * v;
        float bx_moon = 2 * 40.0f - bx; // mirror about center at 40 virtual units.
        ImVec2 moon_center = ToPixels(bx_moon, by, scale, offset_x, offset_y);
        float bodyScale = 0.6f; // Moon is 3x larger.
        float moon_radius_px = (layout.sunDiameter * bodyScale * scale) * 0.5f;
        draw_list->AddCircleFilled(moon_center, moon_radius_px, COLOR_GREEN, 32);
        // Create a crescent shape by overlaying an offset black circle.
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
