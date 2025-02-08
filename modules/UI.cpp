#include "UI.h"
#include <algorithm>
#include <string>

const ImU32 COLOR_GREEN = IM_COL32(0, 255, 0, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

UI::UI() {}
UI::~UI() {}

void UI::Initialize()
{
    // Nothing special
}

void UI::Render(ImDrawList* draw_list,
                BluetoothAudioManager& audioManager,
                Sprite& sprite,
                float scale_x, float scale_y)
{
    /*
      Our virtual space is 80×30. We'll place:

      - Volume bar near the top, but shorter horizontally
      - Artist / track name near the center
      - Progress bar lower (y=22) with the time at (y=23)
      - Car sprite above the bar (using a negative offset)

      Adjust these values if you want slightly different positions.
    */

    // 1) Volume bar near the top
    DrawVolumeBar(draw_list, audioManager, scale_x, scale_y);

    // 2) Artist / track name in the middle region (around y=12..13)
    DrawArtistAndTrackInfo(draw_list, audioManager, scale_x, scale_y);

    // 3) Progress bar near the bottom center, with time below it
    float line_start_x = 15.0f;
    float line_end_x   = 65.0f;
    float line_y       = 22.0f;  // Slightly lower than before
    DrawProgressLine(draw_list, audioManager,
                     line_start_x, line_end_x, line_y,
                     scale_x, scale_y);

    // Place time about 1 unit below
    DrawTimeRemaining(draw_list, audioManager,
                      scale_x, scale_y,
                      line_start_x, line_end_x,
                      line_y + 1.0f);

    // 4) Car sprite above the bar
    //    We'll pass a negative Y offset so it appears above the bar.
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(),
                          line_start_x, line_end_x,
                          scale_x, scale_y,
                          /* x_offset = */ -1.0f,
                          /* y_offset = */ -7.0f); // higher above the line
    sprite.Draw(draw_list, COLOR_GREEN);

    // (Optional) black bars on each end, if you want to hide the sprite
    DrawMaskBars(draw_list, scale_x, scale_y, line_start_x, line_end_x, line_y);
}

void UI::Cleanup()
{
    // Nothing special
}

//--------------------------------------------------------------------------------------
// Single solid progress bar (green line) with a subtle grey behind it
//--------------------------------------------------------------------------------------
void UI::DrawProgressLine(ImDrawList* draw_list,
                          BluetoothAudioManager& audioManager,
                          float line_start_x, float line_end_x, float line_y,
                          float scale_x, float scale_y)
{
    ImVec2 p1 = ToPixels(line_start_x, line_y, scale_x, scale_y);
    ImVec2 p2 = ToPixels(line_end_x,   line_y, scale_x, scale_y);

    // Slight grey behind it
    draw_list->AddLine(p1, p2, IM_COL32(100, 100, 100, 255), 2.0f);
    // Green on top
    draw_list->AddLine(p1, p2, COLOR_GREEN, 4.0f);
}

//--------------------------------------------------------------------------------------
// Black mask bars at each end of the progress line
//--------------------------------------------------------------------------------------
void UI::DrawMaskBars(ImDrawList* draw_list,
                      float scale_x, float scale_y,
                      float line_start_x, float line_end_x, float line_y)
{
    float mask_width  = 1.0f;
    float mask_height = 0.5f;

    ImVec2 left_top  = ToPixels(line_start_x - mask_width,
                                line_y - mask_height, scale_x, scale_y);
    ImVec2 left_bot  = ToPixels(line_start_x,
                                line_y + mask_height, scale_x, scale_y);

    ImVec2 right_top = ToPixels(line_end_x,
                                line_y - mask_height, scale_x, scale_y);
    ImVec2 right_bot = ToPixels(line_end_x + mask_width,
                                line_y + mask_height, scale_x, scale_y);

    draw_list->AddRectFilled(left_top,  left_bot,  COLOR_BLACK);
    draw_list->AddRectFilled(right_top, right_bot, COLOR_BLACK);
}

//--------------------------------------------------------------------------------------
// Time string, placed below the bar. Centered horizontally between line_start_x..line_end_x
//--------------------------------------------------------------------------------------
void UI::DrawTimeRemaining(ImDrawList* draw_list,
                           BluetoothAudioManager& audioManager,
                           float scale_x, float scale_y,
                           float line_start_x, float line_end_x,
                           float line_y)
{
    float region_width = (line_end_x - line_start_x);
    ImVec2 region_pos  = ToPixels(line_start_x, line_y, scale_x, scale_y);

    std::string time_str = audioManager.GetTimeRemaining();
    ImVec2 text_size = ImGui::CalcTextSize(time_str.c_str());
    float region_width_pixels = region_width * scale_x;

    float text_x = region_pos.x + (region_width_pixels - text_size.x) * 0.5f;
    float text_y = region_pos.y;  // same as line_y in pixels
                                  // or add a few more px if you want it lower

    ImGui::SetCursorPos(ImVec2(text_x, text_y));
    ImGui::Text("%s", time_str.c_str());
}

//--------------------------------------------------------------------------------------
// Artist & track name centered around x=40, near y=12..13
//--------------------------------------------------------------------------------------
void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale_x, float scale_y)
{
    const char* artist_name = "Tigers Jaw";
    const char* track_name  = "Never Saw It Coming";

    // We'll define a horizontal region from x=15..65 for center alignment
    float region_left_x = 15.0f;
    float region_right_x= 65.0f;
    float region_width  = region_right_x - region_left_x;

    // Artist near y=12
    float artist_y = 11.0f;
    ImVec2 artist_pos = ToPixels(region_left_x, artist_y, scale_x, scale_y);

    // We'll do a bigger font for the artist
    ImGui::SetWindowFontScale(1.6f);
    ImVec2 text_size = ImGui::CalcTextSize(artist_name);
    float label_width_pixels = region_width * scale_x;

    // Scrolling offset if text is too wide
    static float artist_scroll_offset = 0.0f;
    float dt = ImGui::GetIO().DeltaTime * 30.0f;
    if (text_size.x > label_width_pixels) {
        artist_scroll_offset += dt;
        if (artist_scroll_offset > text_size.x)
            artist_scroll_offset = -label_width_pixels;
    } else {
        artist_scroll_offset = 0.0f;
    }

    ImVec2 clip_min = artist_pos;
    ImVec2 clip_max = ImVec2(artist_pos.x + label_width_pixels,
                             artist_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);

    ImGui::SetCursorPos(ImVec2(artist_pos.x - artist_scroll_offset, artist_pos.y));
    ImGui::TextUnformatted(artist_name);
    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);

    // Track name near y=13
    float track_y = 13.0f;
    ImVec2 track_pos = ToPixels(region_left_x, track_y, scale_x, scale_y);
    ImGui::SetWindowFontScale(1.3f);
    text_size = ImGui::CalcTextSize(track_name);

    static float track_scroll_offset = 0.0f;
    if (text_size.x > label_width_pixels) {
        track_scroll_offset += dt;
        if (track_scroll_offset > text_size.x)
            track_scroll_offset = -label_width_pixels;
    } else {
        track_scroll_offset = 0.0f;
    }

    clip_min = track_pos;
    clip_max = ImVec2(track_pos.x + label_width_pixels,
                      track_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);

    ImGui::SetCursorPos(ImVec2(track_pos.x - track_scroll_offset, track_pos.y));
    ImGui::TextUnformatted(track_name);

    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);
}

//--------------------------------------------------------------------------------------
// Volume bar near the top, but made shorter
//--------------------------------------------------------------------------------------
void UI::DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale_x, float scale_y)
{
    /*
      We'll center the volume bar horizontally by using ~x=25..55
      so it's 30 units wide in virtual coords. "Vol:" is at x=25,
      the bar is next to it, but we won't fill the entire 30. We'll
      make the bar maybe 15 wide in virtual coords. Tweak to taste.
    */
    float region_left_x  = 25.0f;
    float region_right_x = 55.0f;
    float volume_y       = 5.0f;  // near the top

    // Position "Vol:" at left
    ImVec2 label_pos = ToPixels(region_left_x, volume_y, scale_x, scale_y);
    ImGui::SetCursorPos(label_pos);
    ImGui::TextUnformatted("Vol:");
    ImVec2 text_size = ImGui::CalcTextSize("Vol:");

    // Let the bar start a bit to the right
    float bar_x_virtual_start = region_left_x + 7.0f; // 7 units to the right
    float bar_width_virtual   = 15.0f;               // only 15 wide
    float bar_x_virtual_end   = bar_x_virtual_start + bar_width_virtual;

    // Convert to pixels
    float bar_x_start_px = bar_x_virtual_start * scale_x;
    float bar_x_end_px   = bar_x_virtual_end   * scale_x;
    float bar_y_px       = label_pos.y;
    float bar_height_px  = scale_y * 0.8f;

    // Draw the bar
    ImVec2 bar_pos (bar_x_start_px, bar_y_px);
    ImVec2 bar_end (bar_x_end_px,   bar_y_px + bar_height_px);

    draw_list->AddRectFilled(bar_pos, bar_end, COLOR_BLACK);

    float volume_fraction = (float)audioManager.GetVolume() / 128.0f;
    volume_fraction = std::clamp(volume_fraction, 0.0f, 1.0f);

    float fill_x = bar_x_start_px + (bar_x_end_px - bar_x_start_px)*volume_fraction;
    draw_list->AddRectFilled(bar_pos, ImVec2(fill_x, bar_end.y), COLOR_GREEN);
    draw_list->AddRect(bar_pos, bar_end, COLOR_GREEN);

    // Make bar interactive
    ImGui::SetCursorPos(bar_pos);
    ImGui::InvisibleButton("volume_bar", ImVec2(bar_x_end_px - bar_x_start_px,
                                                bar_height_px));
    if (ImGui::IsItemActive()) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float new_frac = (mouse_x - bar_x_start_px) / (bar_x_end_px - bar_x_start_px);
        new_frac = std::clamp(new_frac, 0.0f, 1.0f);
        int new_volume = (int)(new_frac * 128.0f);
        audioManager.SetVolume(new_volume);
    }
}
