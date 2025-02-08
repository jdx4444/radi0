#include "UI.h"
#include <algorithm>
#include <string>

const ImU32 COLOR_GREEN = IM_COL32(0, 255, 0, 255);
const ImU32 COLOR_BLACK = IM_COL32(0, 0, 0, 255);

UI::UI() {}
UI::~UI() {}

void UI::Initialize()
{
    // No changes needed here at the moment
}

void UI::Render(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                Sprite& sprite, float scale_x, float scale_y)
{
    // -------------------------------------------------------------------------
    // CHANGED: Because our virtual coordinate system is 80×30,
    // let's put the progress bar near the bottom: e.g. at y=28
    // We'll run from x=2 to x=78, leaving a margin
    // -------------------------------------------------------------------------
    float line_start_x = 2.0f;
    float line_end_x   = 78.0f;
    float line_y       = 28.0f;

    DrawProgressLine(draw_list, audioManager,
                     line_start_x, line_end_x, line_y,
                     scale_x, scale_y);

    // Move the car sprite along that line
    sprite.UpdatePosition(audioManager.GetPlaybackFraction(),
                          line_start_x, line_end_x,
                          scale_x, scale_y,
                          /* sprite_x_offset = */ -1.5f, // small tweak
                          /* sprite_y_offset = */ -2.0f);

    sprite.Draw(draw_list, COLOR_GREEN);

    // Mask bars to “hide” the edges of the sprite if desired
    DrawMaskBars(draw_list, scale_x, scale_y, line_start_x, line_end_x, line_y);

    DrawTimeRemaining(draw_list, audioManager, scale_x, scale_y);
    DrawArtistAndTrackInfo(draw_list, audioManager, scale_x, scale_y);
    DrawVolumeBar(draw_list, audioManager, scale_x, scale_y);
}

void UI::Cleanup()
{
    // Nothing special
}

void UI::DrawProgressLine(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                          float line_start_x, float line_end_x, float line_y,
                          float scale_x, float scale_y)
{
    // You might want some “background line” plus the green “fill”
    float progress_fraction = audioManager.GetPlaybackFraction();

    ImVec2 p1 = ToPixels(line_start_x, line_y, scale_x, scale_y);
    ImVec2 p2 = ToPixels(line_end_x,   line_y, scale_x, scale_y);

    // Dark grey line behind
    draw_list->AddLine(p1, p2, IM_COL32(80, 80, 80, 255), 6.0f);
    // Green line on top
    float total_length = (p2.x - p1.x);
    ImVec2 p2_fraction = ImVec2(p1.x + total_length * progress_fraction, p2.y);
    draw_list->AddLine(p1, p2_fraction, COLOR_GREEN, 4.0f);
}

// "mask" rectangles near each end of the line, if you want to hide the sprite
// when it goes out of the progress bar. This is optional/flair.
void UI::DrawMaskBars(ImDrawList* draw_list,
                      float scale_x, float scale_y,
                      float line_start_x, float line_end_x, float line_y)
{
    // These are optional black “bars” that hide the sprite at the ends
    float mask_width  = 1.0f;  // 1 virtual unit wide
    float mask_height = 1.0f;  // 1 virtual unit high

    // Left mask
    ImVec2 left_top  = ToPixels(line_start_x - mask_width, line_y - mask_height*0.5f,
                                scale_x, scale_y);
    ImVec2 left_bot  = ToPixels(line_start_x, line_y + mask_height*0.5f,
                                scale_x, scale_y);
    draw_list->AddRectFilled(left_top, left_bot, COLOR_BLACK);

    // Right mask
    ImVec2 right_top = ToPixels(line_end_x, line_y - mask_height*0.5f,
                                scale_x, scale_y);
    ImVec2 right_bot = ToPixels(line_end_x + mask_width, line_y + mask_height*0.5f,
                                scale_x, scale_y);
    draw_list->AddRectFilled(right_top, right_bot, COLOR_BLACK);
}

void UI::DrawTimeRemaining(ImDrawList* draw_list, BluetoothAudioManager& audioManager,
                           float scale_x, float scale_y)
{
    // Let's put the time string slightly above the progress bar
    // Example: at virtual coords (40, 26.5) so it’s near the center
    // Tweak to taste
    ImVec2 timePos = ToPixels(40.0f, 26.5f, scale_x, scale_y);

    // We'll center the text in a small region, so we measure text size
    std::string time_remaining = audioManager.GetTimeRemaining();
    ImVec2 text_size = ImGui::CalcTextSize(time_remaining.c_str());

    // Move the “cursor” so text is centered on (40,26.5)
    ImGui::SetCursorPos(ImVec2(timePos.x - text_size.x*0.5f,
                               timePos.y - text_size.y*0.5f));

    // A slightly bigger font
    ImGui::SetWindowFontScale(1.4f);
    ImGui::TextUnformatted(time_remaining.c_str());
    ImGui::SetWindowFontScale(1.0f);
}

void UI::DrawArtistAndTrackInfo(ImDrawList* draw_list,
                                BluetoothAudioManager& audioManager,
                                float scale_x, float scale_y)
{
    // Example: put the artist name around (10, 10)
    // and track name around (10, 12). Tweak to taste.
    // We'll do some simple scrolling if text is too wide.

    ImVec2 artist_pos = ToPixels(10.0f, 10.0f, scale_x, scale_y);
    ImVec2 track_pos  = ToPixels(10.0f, 12.0f, scale_x, scale_y);

    // Hard-coded for now:
    const char* artist_name = "Tigers Jaw";
    const char* track_name  = "Never Saw It Coming";

    // Let's pick a bigger scale
    float big_font = 1.7f;

    // We can show the artist name with scrolling if it’s too long
    ImGui::SetWindowFontScale(big_font);
    ImVec2 text_size = ImGui::CalcTextSize(artist_name);

    // Define some maximum “label width” in virtual space, say 40 units
    float label_width_in_pixels = 40.0f * scale_x;

    // If text is wider than that, we scroll
    static float artist_scroll_offset = 0.0f;
    float dt = ImGui::GetIO().DeltaTime * 30.0f; // scroll speed
    if (text_size.x > label_width_in_pixels) {
        artist_scroll_offset += dt;
        if (artist_scroll_offset > text_size.x)
            artist_scroll_offset = -label_width_in_pixels;
    } else {
        artist_scroll_offset = 0.0f;
    }

    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 clip_min = ImVec2(artist_pos.x, artist_pos.y);
    ImVec2 clip_max = ImVec2(artist_pos.x + label_width_in_pixels,
                             artist_pos.y + text_size.y);

    draw_list->PushClipRect(clip_min, clip_max, true);

    // Shift the text left by scroll_offset
    ImGui::SetCursorPos(ImVec2(artist_pos.x - artist_scroll_offset,
                               artist_pos.y));
    ImGui::TextUnformatted(artist_name);
    draw_list->PopClipRect();

    ImGui::SetWindowFontScale(1.0f);

    // Now do the track name similarly but a bit smaller
    float track_font = 1.4f;
    ImGui::SetWindowFontScale(track_font);

    text_size = ImGui::CalcTextSize(track_name);
    float track_label_width = 40.0f * scale_x;
    static float track_scroll_offset = 0.0f;

    if (text_size.x > track_label_width) {
        track_scroll_offset += dt;
        if (track_scroll_offset > text_size.x)
            track_scroll_offset = -track_label_width;
    } else {
        track_scroll_offset = 0.0f;
    }

    clip_min = ImVec2(track_pos.x, track_pos.y);
    clip_max = ImVec2(track_pos.x + track_label_width,
                      track_pos.y + text_size.y);
    draw_list->PushClipRect(clip_min, clip_max, true);
    ImGui::SetCursorPos(ImVec2(track_pos.x - track_scroll_offset,
                               track_pos.y));
    ImGui::TextUnformatted(track_name);
    draw_list->PopClipRect();

    ImGui::SetWindowFontScale(1.0f);
}

void UI::DrawVolumeBar(ImDrawList* draw_list,
                       BluetoothAudioManager& audioManager,
                       float scale_x, float scale_y)
{
    // Let’s put the volume near top right. For example, x=65, y=2
    ImVec2 volume_label_pos = ToPixels(65.0f, 2.0f, scale_x, scale_y);
    ImGui::SetCursorPos(volume_label_pos);
    ImGui::TextUnformatted("Vol:");

    // We'll draw a bar next to that
    ImVec2 text_size = ImGui::CalcTextSize("Vol:");
    float bar_x = volume_label_pos.x + text_size.x + 10.0f;
    float bar_y = volume_label_pos.y;

    // Some arbitrary size for the bar
    float bar_width  = 10.0f * scale_x;
    float bar_height = 0.8f * scale_y;

    // We measure the volume fraction
    float volume_fraction = (float)audioManager.GetVolume() / 128.0f;

    ImVec2 bar_pos = ImVec2(bar_x, bar_y);
    ImVec2 bar_end = ImVec2(bar_x + bar_width, bar_y + bar_height);

    draw_list->AddRectFilled(bar_pos, bar_end, COLOR_BLACK);
    draw_list->AddRectFilled(bar_pos,
                             ImVec2(bar_x + bar_width * volume_fraction,
                                    bar_y + bar_height),
                             COLOR_GREEN);
    draw_list->AddRect(bar_pos, bar_end, COLOR_GREEN);

    // Make it clickable/dragable
    ImGui::SetCursorPos(bar_pos);
    ImGui::InvisibleButton("volume_bar", ImVec2(bar_width, bar_height));
    if (ImGui::IsItemActive()) {
        float mouse_x = ImGui::GetIO().MousePos.x;
        float new_frac = (mouse_x - bar_pos.x) / bar_width;
        new_frac = std::clamp(new_frac, 0.0f, 1.0f);
        int new_vol = (int)(new_frac * 128.0f);
        audioManager.SetVolume(new_vol);
    }
}
