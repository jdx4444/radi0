#include "UI.h"
#include <algorithm>
#include <string>

// Colors
const ImU32 COLOR_GREEN = IM_COL32(0, 255, 0, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

UI::UI() {}
UI::~UI() {}

void UI::Initialize() {
    // Nothing special needed here
}

void UI::Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale_x, float scale_y)
{
    // 1) Volume bar near the top
    DrawVolumeBar(draw_list, audioManager, scale_x, scale_y);

    // 2) Artist / track info in the middle region
    DrawArtistAndTrackInfo(draw_list, audioManager, scale_x, scale_y);

    // 3) Progress bar near the bottom center, with time below it
    DrawProgressLine(draw_list, audioManager, scale_x, scale_y);
    DrawTimeRemaining(draw_list, audioManager, scale_x, scale_y);

    // 4) Car sprite above the progress bar
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(),
                          layout.progressBarStartX, layout.progressBarEndX,
                          scale_x, scale_y,
                          layout.spriteXOffset,
                          layout.spriteYOffset);
    sprite.Draw(draw_list, COLOR_GREEN);

    // Optional: black mask bars on each end
    DrawMaskBars(draw_list, scale_x, scale_y);
}

void UI::Cleanup() {
    // Nothing special to clean up
}

//------------------------------------------------------------------------------
// DrawVolumeBar: Renders the volume label and an interactive volume bar.
//------------------------------------------------------------------------------
void UI::DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale_x, float scale_y)
{
    // Draw the "Vol:" label
    float label_virtual_x = layout.volumeLabelX;
    float volume_virtual_y  = layout.volumeLabelY;
    ImVec2 label_pos = ToPixels(label_virtual_x, volume_virtual_y, scale_x, scale_y);
    ImGui::SetCursorPos(label_pos);
    ImGui::TextUnformatted("Vol:");
    ImVec2 text_size = ImGui::CalcTextSize("Vol:");

    // Compute the volume bar’s start and end in virtual coordinates
    float bar_virtual_start = label_virtual_x + layout.volumeBarOffset;
    float bar_virtual_end   = bar_virtual_start + layout.volumeBarWidth;

    // Convert to pixels
    float bar_x_start_px = bar_virtual_start * scale_x;
    float bar_x_end_px   = bar_virtual_end   * scale_x;
    float bar_y_px       = label_pos.y;
    float bar_height_px  = layout.volumeBarHeight * scale_y;

    // Draw the empty bar (background)
    ImVec2 bar_pos(bar_x_start_px, bar_y_px);
    ImVec2 bar_end(bar_x_end_px, bar_y_px + bar_height_px);
    draw_list->AddRectFilled(bar_pos, bar_end, COLOR_BLACK);

    // Calculate filled fraction based on volume (0-128)
    float volume_fraction = static_cast<float>(audioManager.GetVolume()) / 128.0f;
    volume_fraction = std::clamp(volume_fraction, 0.0f, 1.0f);

    float fill_x = bar_x_start_px + (bar_x_end_px - bar_x_start_px) * volume_fraction;
    draw_list->AddRectFilled(bar_pos, ImVec2(fill_x, bar_end.y), COLOR_GREEN);
    draw_list->AddRect(bar_pos, bar_end, COLOR_GREEN);

    // Make the bar interactive
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

//------------------------------------------------------------------------------
// DrawArtistAndTrackInfo: Renders scrolling artist and track name.
//------------------------------------------------------------------------------
void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale_x, float scale_y)
{
    const char* artist_name = "Tigers Jaw";
    const char* track_name  = "Never Saw It Coming";

    float region_left_x = layout.trackRegionLeftX;
    float region_right_x = layout.trackRegionRightX;
    float region_width = region_right_x - region_left_x;

    // --- Draw Artist Name ---
    float artist_virtual_y = layout.artistY;
    ImVec2 artist_pos = ToPixels(region_left_x, artist_virtual_y, scale_x, scale_y);
    ImGui::SetWindowFontScale(1.6f);
    ImVec2 text_size = ImGui::CalcTextSize(artist_name);
    float region_width_px = region_width * scale_x;

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
    ImGui::TextUnformatted(artist_name);
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);

    // --- Draw Track Name ---
    float track_virtual_y = layout.trackY;
    ImVec2 track_pos = ToPixels(region_left_x, track_virtual_y, scale_x, scale_y);
    ImGui::SetWindowFontScale(1.3f);
    text_size = ImGui::CalcTextSize(track_name);

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
    ImGui::TextUnformatted(track_name);
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);
}

//------------------------------------------------------------------------------
// DrawProgressLine: Renders the progress line as a green line.
//------------------------------------------------------------------------------
void UI::DrawProgressLine(ImDrawList* draw_list,
                          BluetoothAudioManager& audioManager,
                          float scale_x, float scale_y)
{
    ImVec2 p1 = ToPixels(layout.progressBarStartX, layout.progressBarY, scale_x, scale_y);
    ImVec2 p2 = ToPixels(layout.progressBarEndX, layout.progressBarY, scale_x, scale_y);
    draw_list->AddLine(p1, p2, COLOR_GREEN, 4.0f);
}

//------------------------------------------------------------------------------
// DrawTimeRemaining: Renders the remaining time text centered below the progress line.
//------------------------------------------------------------------------------
void UI::DrawTimeRemaining(ImDrawList* draw_list,
                           BluetoothAudioManager& audioManager,
                           float scale_x, float scale_y)
{
    float region_width = layout.progressBarEndX - layout.progressBarStartX;
    ImVec2 region_pos = ToPixels(layout.progressBarStartX, layout.progressBarY + layout.timeTextYOffset, scale_x, scale_y);

    std::string time_str = audioManager.GetTimeRemaining();
    ImVec2 text_size = ImGui::CalcTextSize(time_str.c_str());
    float region_width_pixels = region_width * scale_x;

    float text_x = region_pos.x + (region_width_pixels - text_size.x) * 0.5f;
    float text_y = region_pos.y;
    ImGui::SetCursorPos(ImVec2(text_x, text_y));
    ImGui::Text("%s", time_str.c_str());
}

//------------------------------------------------------------------------------
// DrawMaskBars: Optionally draw black rectangles at each end of the progress line.
//------------------------------------------------------------------------------
void UI::DrawMaskBars(ImDrawList* draw_list,
                      float scale_x, float scale_y)
{
    float mask_width  = 1.0f;
    float mask_height = 1.0f;

    ImVec2 left_top = ToPixels(layout.progressBarStartX - mask_width, layout.progressBarY - mask_height, scale_x, scale_y);
    ImVec2 left_bot = ToPixels(layout.progressBarStartX, layout.progressBarY + mask_height, scale_x, scale_y);

    ImVec2 right_top = ToPixels(layout.progressBarEndX, layout.progressBarY - mask_height, scale_x, scale_y);
    ImVec2 right_bot = ToPixels(layout.progressBarEndX + mask_width, layout.progressBarY + mask_height, scale_x, scale_y);

    draw_list->AddRectFilled(left_top,  left_bot,  COLOR_BLACK);
    draw_list->AddRectFilled(right_top, right_bot, COLOR_BLACK);
}
