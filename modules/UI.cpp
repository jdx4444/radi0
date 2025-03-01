#include "UI.h"
#include <algorithm>
#include <string>

// Define some colors (using the new hex #6dfe95)
const ImU32 COLOR_GREEN = IM_COL32(109, 254, 149, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

UI::UI() {}
UI::~UI() {}

void UI::Initialize() {
    // No special initialization needed
}

void UI::Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale,
                float offset_x,
                float offset_y)
{
    // 1) Draw the volume bar near the top
    DrawVolumeBar(draw_list, audioManager, scale, offset_x, offset_y);

    // 2) Draw scrolling artist and track info using metadata from BluetoothAudioManager
    DrawArtistAndTrackInfo(draw_list, audioManager, scale, offset_x, offset_y);

    // 3) Draw the progress line and remaining time
    DrawProgressLine(draw_list, audioManager, scale, offset_x, offset_y);
    DrawTimeRemaining(draw_list, audioManager, scale, offset_x, offset_y);

    // 4) Update and draw the car sprite above the progress bar
    constexpr float spriteVirtualWidth = 19 * 0.16f; // Approximately 3.04 virtual units
    float effectiveStartX = layout.progressBarStartX - spriteVirtualWidth;
    float effectiveEndX   = layout.progressBarEndX;
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(),
                          effectiveStartX, effectiveEndX,
                          scale, offset_x, offset_y,
                          layout.spriteXOffset,
                          layout.spriteYOffset,
                          layout.spriteBaseY);
    sprite.Draw(draw_list, COLOR_GREEN);

    // 5) Draw mask bars to conceal sprite edges if needed
    DrawMaskBars(draw_list, scale, offset_x, offset_y);
}

void UI::Cleanup() {
    // No cleanup needed
}

void UI::DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale,
                       float offset_x,
                       float offset_y)
{
    // Draw the "Vol:" label.
    float label_virtual_x = layout.volumeLabelX;
    float volume_virtual_y  = layout.volumeLabelY;
    ImVec2 label_pos = ToPixels(label_virtual_x, volume_virtual_y, scale, offset_x, offset_y);
    ImGui::SetCursorPos(label_pos);
    ImGui::TextUnformatted("Vol:");
    ImVec2 text_size = ImGui::CalcTextSize("Vol:");

    // Compute the volume bar start and end positions in virtual coordinates.
    float bar_virtual_start = label_virtual_x + layout.volumeBarOffset;
    float bar_virtual_end   = bar_virtual_start + layout.volumeBarWidth;

    // Convert to pixels.
    float bar_x_start_px = bar_virtual_start * scale + offset_x;
    float bar_x_end_px   = bar_virtual_end   * scale + offset_x;
    float bar_y_px       = label_pos.y;
    float bar_height_px  = layout.volumeBarHeight * scale;

    // Draw the empty (black) bar.
    ImVec2 bar_pos(bar_x_start_px, bar_y_px);
    ImVec2 bar_end(bar_x_end_px, bar_y_px + bar_height_px);
    draw_list->AddRectFilled(bar_pos, bar_end, COLOR_BLACK);

    // Draw the filled portion based on volume (0-128)
    float volume_fraction = static_cast<float>(audioManager.GetVolume()) / 128.0f;
    volume_fraction = std::clamp(volume_fraction, 0.0f, 1.0f);
    float fill_x = bar_x_start_px + (bar_x_end_px - bar_x_start_px) * volume_fraction;
    draw_list->AddRectFilled(bar_pos, ImVec2(fill_x, bar_end.y), COLOR_GREEN);
    draw_list->AddRect(bar_pos, bar_end, COLOR_GREEN);

    // Make the bar interactive.
    ImGui::SetCursorPos(bar_pos);
    ImGui::InvisibleButton("volume_bar", ImVec2(bar_x_end_px - bar_x_start_px, bar_height_px));
    if (ImGui::IsItemActive()) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float new_frac = (mouse_x - bar_x_start_px) / (bar_x_end_px - bar_x_start_px);
        new_frac = std::clamp(new_frac, 0.0f, 1.0f);
        int new_volume = static_cast<int>(new_frac * 128.0f);
        audioManager.SetVolume(new_volume);
    }
}

void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale,
                                float offset_x,
                                float offset_y)
{
    // Fetch metadata from BluetoothAudioManager.
    std::string artist_name = audioManager.GetCurrentTrackArtist();
    std::string track_name  = audioManager.GetCurrentTrackTitle();
    if (artist_name.empty())
         artist_name = "Unknown Artist";
    if (track_name.empty())
         track_name = "Unknown Track";

    float region_left_x = layout.trackRegionLeftX;
    float region_right_x = layout.trackRegionRightX;
    float region_width = region_right_x - region_left_x;

    // --- Draw Artist Name ---
    float artist_virtual_y = layout.artistY;
    ImVec2 artist_pos = ToPixels(region_left_x, artist_virtual_y, scale, offset_x, offset_y);
    ImGui::SetWindowFontScale(1.6f);
    ImVec2 text_size = ImGui::CalcTextSize(artist_name.c_str());
    float region_width_px = region_width * scale;

    static float artist_scroll_offset = 0.0f;
    float dt = ImGui::GetIO().DeltaTime * 30.0f;
    if (text_size.x > region_width_px) {
        artist_scroll_offset += dt;
        if (artist_scroll_offset > text_size.x)
            artist_scroll_offset = -region_width_px;
    } else {
        artist_scroll_offset = 0.0f;
    }

    ImVec2 clip_min = artist_pos;
    ImVec2 clip_max = ImVec2(artist_pos.x + region_width_px, artist_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);
    ImGui::SetCursorPos(ImVec2(artist_pos.x - artist_scroll_offset, artist_pos.y));
    ImGui::TextUnformatted(artist_name.c_str());
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);

    // --- Draw Track Name ---
    float track_virtual_y = layout.trackY;
    ImVec2 track_pos = ToPixels(region_left_x, track_virtual_y, scale, offset_x, offset_y);
    ImGui::SetWindowFontScale(1.3f);
    text_size = ImGui::CalcTextSize(track_name.c_str());
    static float track_scroll_offset = 0.0f;
    if (text_size.x > region_width_px) {
        track_scroll_offset += dt;
        if (track_scroll_offset > text_size.x)
            track_scroll_offset = -region_width_px;
    } else {
        track_scroll_offset = 0.0f;
    }
    clip_min = track_pos;
    clip_max = ImVec2(track_pos.x + region_width_px, track_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);
    ImGui::SetCursorPos(ImVec2(track_pos.x - track_scroll_offset, track_pos.y));
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

    // Use the virtual thickness from LayoutConfig so that at your baseline display (e.g. scale = 16)
    // the progress line is exactly 4 px thick, but it scales proportionally on other displays.
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
    ImVec2 region_pos = ToPixels(layout.progressBarStartX, layout.progressBarY + layout.timeTextYOffset, scale, offset_x, offset_y);
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
    float mask_width  = layout.maskBarWidth;
    float mask_height = layout.maskBarHeight;
    ImVec2 left_top  = ToPixels(layout.progressBarStartX - mask_width, layout.progressBarY - mask_height, scale, offset_x, offset_y);
    ImVec2 left_bot  = ToPixels(layout.progressBarStartX, layout.progressBarY + mask_height, scale, offset_x, offset_y);
    ImVec2 right_top = ToPixels(layout.progressBarEndX, layout.progressBarY - mask_height, scale, offset_x, offset_y);
    ImVec2 right_bot = ToPixels(layout.progressBarEndX + mask_width, layout.progressBarY + mask_height, scale, offset_x, offset_y);
    draw_list->AddRectFilled(left_top, left_bot, COLOR_BLACK);
    draw_list->AddRectFilled(right_top, right_bot, COLOR_BLACK);
}
