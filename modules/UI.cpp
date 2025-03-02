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

    // 4) Update and draw the car sprite above the progress bar.
    // Adjust the effective start and end positions using spriteXCorrection.
    constexpr float spriteVirtualWidth = 19 * 0.16f; // Approximately 3.04 virtual units
    float effectiveStartX = layout.progressBarStartX - spriteVirtualWidth + layout.spriteXCorrection;
    float effectiveEndX   = layout.progressBarEndX + layout.spriteXCorrection;
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

    // Instead of using a fixed volumeBarOffset, we now add a small gap (volumeBarTextGap)
    // to separate the label from the bar.
    float gap_pixels = layout.volumeBarTextGap * scale;
    // Compute the virtual starting position for the volume bar based on the label's end.
    float bar_virtual_start = ((label_pos.x + text_size.x + gap_pixels) - offset_x) / scale;
    float bar_x_start_px = bar_virtual_start * scale + offset_x;
    // Total width (in pixels) remains as defined in the layout.
    float total_bar_width_px = layout.volumeBarWidth * scale;
    
    // Draw the segmented volume bar:
    int cellCount = layout.volumeCellCount;
    float gap_between_cells = layout.volumeCellGap * scale; // gap in pixels between cells
    // Calculate each cell's width in pixels.
    float cell_width = (total_bar_width_px - (cellCount - 1) * gap_between_cells) / cellCount;
    float bar_y_px = label_pos.y; // use same vertical position as label
    float bar_height_px = layout.volumeBarHeight * scale;
    
    // Determine how many cells should be filled based on the volume (0-128).
    int volume = audioManager.GetVolume();
    int filled_cells = static_cast<int>((volume / 128.0f) * cellCount);
    
    // Draw each cell (filled or just outlined).
    for (int i = 0; i < cellCount; ++i) {
        float cell_x = bar_x_start_px + i * (cell_width + gap_between_cells);
        ImVec2 cell_top_left(cell_x, bar_y_px);
        ImVec2 cell_bottom_right(cell_x + cell_width, bar_y_px + bar_height_px);
        
        if (i < filled_cells) {
            // Draw filled cell.
            draw_list->AddRectFilled(cell_top_left, cell_bottom_right, COLOR_GREEN);
        } else {
            // Draw an outlined cell.
            draw_list->AddRect(cell_top_left, cell_bottom_right, COLOR_GREEN);
        }
    }
    
    // Optionally, you could also make the volume bar interactive by using an invisible button,
    // similar to the previous implementation.
    ImGui::SetCursorPos(ImVec2(bar_x_start_px, bar_y_px));
    ImGui::InvisibleButton("volume_bar", ImVec2(total_bar_width_px, bar_height_px));
    if (ImGui::IsItemActive()) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        // Calculate fraction based on mouse position relative to the bar.
        float new_frac = (mouse_x - bar_x_start_px) / total_bar_width_px;
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

    // The progress line thickness is determined by a virtual value scaled up.
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
