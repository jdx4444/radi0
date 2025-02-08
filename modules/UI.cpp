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
      We have an 80×30 virtual canvas (from your main.cpp).
      We'll define a horizontal “center region” from x=15..65 (50 wide),
      and place the progress bar around y=18 for a near‐centered look.
      We'll stack the artist name / track name above it, and
      the volume bar somewhat above that as well.

      You can tweak these numbers however you like—this is just
      a simple example to get everything centered-ish.
    */

    // 1) Progress bar
    float line_start_x = 15.0f; // left edge
    float line_end_x   = 65.0f; // right edge
    float line_y       = 18.0f; // near vertical center
    DrawProgressLine(draw_list, audioManager,
                     line_start_x, line_end_x, line_y,
                     scale_x, scale_y);

    // Place the time underneath (about 1 unit below)
    DrawTimeRemaining(draw_list, audioManager,
                      scale_x, scale_y,
                      line_start_x, line_end_x,
                      line_y + 1.0f);

    // 2) Sprite on the bar
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(),
                          line_start_x, line_end_x,
                          scale_x, scale_y,
                          /* xOffset=*/-1.0f,
                          /* yOffset=*/-4.0f);
    sprite.Draw(draw_list, COLOR_GREEN);

    // (Optional) mask bars so the sprite doesn't peek out the ends
    DrawMaskBars(draw_list, scale_x, scale_y,
                 line_start_x, line_end_x, line_y);

    // 3) Artist / track name above the bar
    DrawArtistAndTrackInfo(draw_list, audioManager, scale_x, scale_y);

    // 4) Volume bar also near the center (above everything else)
    DrawVolumeBar(draw_list, audioManager, scale_x, scale_y);
}

void UI::Cleanup()
{
    // Nothing special
}

//--------------------------------------------------------------------------------------
// REVERTED: single-line progress bar from the original
//--------------------------------------------------------------------------------------
void UI::DrawProgressLine(ImDrawList* draw_list,
                          BluetoothAudioManager& audioManager,
                          float line_start_x, float line_end_x, float line_y,
                          float scale_x, float scale_y)
{
    // Convert virtual coords → screen
    ImVec2 p1 = ToPixels(line_start_x, line_y, scale_x, scale_y);
    ImVec2 p2 = ToPixels(line_end_x,   line_y, scale_x, scale_y);

    // A single solid green line (with a slight grey behind it, as originally)
    // The original code had two lines to create a green “thickness.”
    // “2.0f” was the grey line thickness, then “4.0f” for green on top.

    draw_list->AddLine(p1, p2, IM_COL32(100, 100, 100, 255), 2.0f); // behind
    draw_list->AddLine(p1, p2, COLOR_GREEN, 4.0f);                  // on top
}

//--------------------------------------------------------------------------------------
// (Optional) mask rectangles at each end of the line to hide sprite edges
//--------------------------------------------------------------------------------------
void UI::DrawMaskBars(ImDrawList* draw_list,
                      float scale_x, float scale_y,
                      float line_start_x, float line_end_x, float line_y)
{
    // Same as original approach: just some small black bars at the ends
    // so the car sprite doesn’t appear “off” the line.
    // Tweak as you like or remove if you don’t need it.
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
// Put the time *under* the bar (reverted to the original logic).
//--------------------------------------------------------------------------------------
void UI::DrawTimeRemaining(ImDrawList* draw_list,
                           BluetoothAudioManager& audioManager,
                           float scale_x, float scale_y,
                           float line_start_x, float line_end_x,
                           float line_y)
{
    // We'll place the text in the same horizontal span as the bar (15..65)
    // but at line_y in “virtual coords.” We want it centered horizontally
    // within that region. The original logic used a small “progress_bar_size,”
    // but we can do it directly here.

    float region_width = (line_end_x - line_start_x);

    // Where we want the top-left of the text “region” to be
    // (We’ll offset it slightly downward if you like.)
    ImVec2 region_pos = ToPixels(line_start_x, line_y, scale_x, scale_y);

    // The time string
    std::string time_str = audioManager.GetTimeRemaining();
    ImVec2 text_size = ImGui::CalcTextSize(time_str.c_str());

    // Compute how many pixels wide the bar region is
    float region_width_pixels = region_width * scale_x;

    // The x pos that will center the text in that region
    float text_x = region_pos.x + (region_width_pixels - text_size.x) * 0.5f;

    // If you want the text slightly below the bar, just add a few pixels:
    float text_y = region_pos.y + (5.0f);

    // Position the text
    ImGui::SetCursorPos(ImVec2(text_x, text_y));
    ImGui::Text("%s", time_str.c_str());
}

//--------------------------------------------------------------------------------------
// Center the artist & track name. 
// We’ll place them a bit *above* the progress bar in the vertical center region.
//--------------------------------------------------------------------------------------
void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale_x, float scale_y)
{
    // We'll pick some coordinates so that the text is centered horizontally at x=40,
    // and near y=12..13 in your 80×30 space. We also do horizontal “clipping” if text is too wide.

    // Hard-coded sample artist & track
    const char* artist_name = "Tigers Jaw";
    const char* track_name  = "Never Saw It Coming";

    // We'll define a region from x=15..65 again (the same 50 wide region we used for the bar),
    // but place them around y=12 and y=13, respectively.
    float region_left_x = 15.0f;
    float region_right_x = 65.0f;
    float region_width = region_right_x - region_left_x;
    
    // Artist near y=12
    float artist_y = 12.0f;
    ImVec2 artist_pos = ToPixels(region_left_x, artist_y, scale_x, scale_y);
    float label_width_in_pixels = region_width * scale_x;

    ImGui::SetWindowFontScale(1.4f); // bigger text
    ImVec2 text_size = ImGui::CalcTextSize(artist_name);

    // We can do simple scrolling if text is too wide
    static float artist_scroll_offset = 0.0f;
    float dt = ImGui::GetIO().DeltaTime * 30.0f; // scroll speed
    if (text_size.x > label_width_in_pixels) {
        artist_scroll_offset += dt;
        if (artist_scroll_offset > text_size.x)
            artist_scroll_offset = -label_width_in_pixels;
    } else {
        artist_scroll_offset = 0.0f;
    }

    // Clip rect
    ImVec2 clip_min = artist_pos;
    ImVec2 clip_max = ImVec2(artist_pos.x + label_width_in_pixels,
                             artist_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);

    // Move the text left by offset
    ImGui::SetCursorPos(ImVec2(artist_pos.x - artist_scroll_offset,
                               artist_pos.y));
    ImGui::TextUnformatted(artist_name);
    draw_list->PopClipRect();

    ImGui::SetWindowFontScale(1.0f);

    // Track name near y=13
    float track_y = 13.0f;
    ImVec2 track_pos = ToPixels(region_left_x, track_y, scale_x, scale_y);
    ImGui::SetWindowFontScale(1.2f);
    text_size = ImGui::CalcTextSize(track_name);

    static float track_scroll_offset = 0.0f;
    if (text_size.x > label_width_in_pixels) {
        track_scroll_offset += dt;
        if (track_scroll_offset > text_size.x)
            track_scroll_offset = -label_width_in_pixels;
    } else {
        track_scroll_offset = 0.0f;
    }

    clip_min = track_pos;
    clip_max = ImVec2(track_pos.x + label_width_in_pixels,
                      track_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);

    ImGui::SetCursorPos(ImVec2(track_pos.x - track_scroll_offset, track_pos.y));
    ImGui::TextUnformatted(track_name);

    draw_list->PopClipRect();
    ImGui::SetWindowFontScale(1.0f);
}

//--------------------------------------------------------------------------------------
// Center the volume bar horizontally near the top (y=7).
//--------------------------------------------------------------------------------------
void UI::DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale_x, float scale_y)
{
    // We'll use the same horizontal region from x=15..65.
    float region_left_x = 15.0f;
    float region_right_x = 65.0f;
    float region_width = region_right_x - region_left_x;
    float volume_y = 7.0f;

    // The label "Vol:" is placed at the left edge, and the bar to the right of that
    ImVec2 label_pos = ToPixels(region_left_x, volume_y, scale_x, scale_y);
    ImGui::SetCursorPos(label_pos);
    ImGui::TextUnformatted("Vol:");
    ImVec2 text_size = ImGui::CalcTextSize("Vol:");

    float bar_x = label_pos.x + text_size.x + 10.0f; // a small gap
    float bar_y = label_pos.y;

    // Let the bar fill the remainder up to region_right_x
    float bar_width_pixels = (region_right_x * scale_x) - bar_x;
    if (bar_width_pixels < 0.0f) bar_width_pixels = 0.0f;
    float bar_height_pixels = scale_y * 0.6f; // arbitrary

    // Volume fraction
    float volume_fraction = (float)audioManager.GetVolume() / 128.0f;
    volume_fraction = std::clamp(volume_fraction, 0.0f, 1.0f);

    ImVec2 bar_pos  = ImVec2(bar_x, bar_y);
    ImVec2 bar_end  = ImVec2(bar_x + bar_width_pixels, bar_y + bar_height_pixels);

    // Black background
    draw_list->AddRectFilled(bar_pos, bar_end, COLOR_BLACK);
    // Green fill
    float fill_x = bar_x + bar_width_pixels * volume_fraction;
    draw_list->AddRectFilled(bar_pos, ImVec2(fill_x, bar_end.y), COLOR_GREEN);
    // Outline
    draw_list->AddRect(bar_pos, bar_end, COLOR_GREEN);

    // Make it draggable
    ImGui::SetCursorPos(bar_pos);
    ImGui::InvisibleButton("volume_bar", ImVec2(bar_width_pixels, bar_height_pixels));
    if (ImGui::IsItemActive()) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float new_frac = (mouse_x - bar_pos.x) / bar_width_pixels;
        new_frac = std::clamp(new_frac, 0.0f, 1.0f);
        int new_volume = (int)(new_frac * 128.0f);
        audioManager.SetVolume(new_volume);
    }
}
